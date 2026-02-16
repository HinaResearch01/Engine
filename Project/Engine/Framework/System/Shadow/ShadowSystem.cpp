#include "ShadowSystem.h"
#include "Framework/World/World.h"
#include "Framework/System/Camera/CameraSystem.h"
#include "Framework/Context/CameraContext.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#undef min
#undef max

using namespace Tsumi;
using namespace Framework;

static Math::Vec3f NormalizeSafe(const Math::Vec3f& v, const Math::Vec3f& fallback)
{
	const float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
	if (len2 <= 1e-12f) return fallback;
	const float invLen = 1.0f / std::sqrt(len2);
	return { v.x * invLen, v.y * invLen, v.z * invLen };
}

static float Clamp(float x, float a, float b)
{
	return std::max(a, std::min(x, b));
}
static void GetFrustumCornersWS(
	const Math::Mat4x4& invViewProj,
	float ndcNearZ, float ndcFarZ,
	Math::Vec3f outCorners[8]
)

{
	// D3D NDC: x,y [-1..1], z [0..1]
	const float xs[2] = { -1.0f, 1.0f };
	const float ys[2] = { -1.0f, 1.0f };

	int idx = 0;
	for (int y = 0; y < 2; ++y)
		for (int x = 0; x < 2; ++x)
		{
			// near
			{
				Math::Vec4f c = { xs[x], ys[y], ndcNearZ, 1.0f };
				Math::Vec4f w = invViewProj * c;
				const float iw = (std::abs(w.w) > 1e-6f) ? (1.0f / w.w) : 1.0f;
				outCorners[idx++] = { w.x * iw, w.y * iw, w.z * iw };
			}
		}
	for (int y = 0; y < 2; ++y)
		for (int x = 0; x < 2; ++x)
		{
			// far
			{
				Math::Vec4f c = { xs[x], ys[y], ndcFarZ, 1.0f };
				Math::Vec4f w = invViewProj * c;
				const float iw = (std::abs(w.w) > 1e-6f) ? (1.0f / w.w) : 1.0f;
				outCorners[idx++] = { w.x * iw, w.y * iw, w.z * iw };
			}
		}
}

static Math::Vec3f Average8(const Math::Vec3f p[8])
{
	Math::Vec3f s{ 0,0,0 };
	for (int i = 0; i < 8; ++i) { s.x += p[i].x; s.y += p[i].y; s.z += p[i].z; }
	return { s.x / 8.0f, s.y / 8.0f, s.z / 8.0f };
}

ShadowSystem::ShadowSystem(World& world)
	: world_(world)
{}

void ShadowSystem::Init()
{
	BuildDefault(defaultCtx_);
	activeCtx_ = defaultCtx_;
}

void ShadowSystem::Update(float)
{
	ShadowContext ctx{};
	BuildShadowContext(ctx);

	// 「影が組めない」ならデフォルトにフォールバック
	if (!ctx.enabled)
	{
		activeCtx_ = defaultCtx_;
	}
	else
	{
		activeCtx_ = ctx;
	}
}

void ShadowSystem::BuildShadowContext(ShadowContext& out)
{
	out = {};
	out.enabled = false;

	// Camera
	auto camSys = world_.GetSystem<CameraSystem>();
	const CameraContext& cam = camSys->GetContext();
	if (!cam.valid)
	{
		return;
	}

	// Choose 1 directional light
	const DirectionalLightComponent* chosenDL = nullptr;
	const TransformComponent* chosenTR = nullptr;

	for (auto [tr, dl] : world_.View<TransformComponent, DirectionalLightComponent>())
	{
		if (!dl.shadow.castShadow) continue;
		if (dl.intensity <= 0.0f) continue;
		if (dl.shadow.shadowMapSize == 0) continue;

		chosenDL = &dl;
		chosenTR = &tr;
		break;
	}

	if (!chosenDL || !chosenTR)
	{
		// 影を組めない → enabled=falseのまま（Update側でdefaultへ）
		return;
	}

	out.enabled = true;
	out.shadowMapSize = chosenDL->shadow.shadowMapSize;

	// CSM splits
	constexpr uint32_t cascadeCount = 4;
	out.cascadeCount = cascadeCount;

	const float lambda = 0.7f;
	const float n = cam.nearPlane;
	const float f = cam.farPlane;

	float splits[5]{};
	splits[0] = n;
	for (int i = 1; i <= 4; ++i)
	{
		const float p = (float)i / 4.0f;
		const float logSplit = n * std::pow(f / n, p);
		const float uniSplit = n + (f - n) * p;
		splits[i] = lambda * logSplit + (1.0f - lambda) * uniSplit;
	}

	for (uint32_t i = 0; i < cascadeCount; ++i)
	{
		out.splitFar[i] = splits[i + 1];
	}

	// Light dir
	Math::Vec3f lightDirWS = NormalizeSafe(-chosenTR->forward, { 0,-1,0 });

	Math::Vec3f up{ 0,1,0 };
	const float dotUp = lightDirWS.x * up.x + lightDirWS.y * up.y + lightDirWS.z * up.z;
	if (std::abs(dotUp) > 0.99f) up = { 1,0,0 };

	// Each cascade build ortho
	for (uint32_t ci = 0; ci < cascadeCount; ++ci)
	{
		const float cn = splits[ci];
		const float cf = splits[ci + 1];

		const Math::Mat4x4 cascadeProj =
			Math::Func::MAT4x4::PerspectiveFovMatrix(cam.fovY, cam.aspectRatio, cn, cf);

		const Math::Mat4x4 cascadeVP = cam.view * cascadeProj;
		const Math::Mat4x4 invCascadeVP = cascadeVP.Inverse();

		Math::Vec3f cornersWS[8]{};
		GetFrustumCornersWS(invCascadeVP, 0.0f, 1.0f, cornersWS);

		const Math::Vec3f centerWS = Average8(cornersWS);

		const float dist = (cf - cn) + chosenDL->shadow.orthoHalfSize;
		const Math::Vec3f lightPosWS = {
			centerWS.x - lightDirWS.x * dist,
			centerWS.y - lightDirWS.y * dist,
			centerWS.z - lightDirWS.z * dist
		};

		Math::Mat4x4 lightView = Math::Func::MAT4x4::LookAtLH(lightPosWS, centerWS, up);

		// corners -> light space AABB
		Math::Vec3f minLS{ +1e30f, +1e30f, +1e30f };
		Math::Vec3f maxLS{ -1e30f, -1e30f, -1e30f };

		for (int i = 0; i < 8; ++i)
		{
			const Math::Vec3f pLS = lightView.TransformPoint(cornersWS[i]);
			minLS.x = std::min(minLS.x, pLS.x);
			minLS.y = std::min(minLS.y, pLS.y);
			minLS.z = std::min(minLS.z, pLS.z);
			maxLS.x = std::max(maxLS.x, pLS.x);
			maxLS.y = std::max(maxLS.y, pLS.y);
			maxLS.z = std::max(maxLS.z, pLS.z);
		}

		// pad
		const float padXY = 10.0f;
		const float padZ = 300.0f; // 手前のオブジェクトも影を落とせるように大きく取る
		minLS.x -= padXY; minLS.y -= padXY;
		maxLS.x += padXY; maxLS.y += padXY;
		minLS.z -= padZ;
		maxLS.z += padZ;

        // texel snap
        //const float orthoW = (maxLS.x - minLS.x);
        //const float orthoH = (maxLS.y - minLS.y);
        // SnapOrthoToTexel(lightView, orthoW, orthoH, out.shadowMapSize);

		const Math::Mat4x4 lightProj = Math::Func::MAT4x4::OrthographicMatrix(
			minLS.x, maxLS.x,
			minLS.y, maxLS.y,
			minLS.z, maxLS.z
		);

		auto& c = out.cascades[ci];
		c.view = lightView;
		c.proj = lightProj;
		c.viewProj = lightView * lightProj;
	}
}

void ShadowSystem::BuildDefault(ShadowContext& out)
{
	out = {};
	out.enabled = true;

	out.shadowMapSize = 1024;
	out.cascadeCount = 1;

	// Camera
	auto camSys = world_.GetSystem<CameraSystem>();
	const CameraContext& cam = camSys->GetContext();

	// 仮ライト方向
	Math::Vec3f lightDirWS = NormalizeSafe({ 0.3f, -1.0f, 0.2f }, { 0,-1,0 });

	// up決定
	Math::Vec3f up{ 0,1,0 };
	const float dotUp = lightDirWS.x * up.x + lightDirWS.y * up.y + lightDirWS.z * up.z;
	if (std::abs(dotUp) > 0.99f) up = { 1,0,0 };

	// カメラ位置付近を中心に固定のオルソを作る
	const Math::Vec3f centerWS = cam.position;

	const float dist = 60.0f;
	const Math::Vec3f lightPosWS = {
		centerWS.x - lightDirWS.x * dist,
		centerWS.y - lightDirWS.y * dist,
		centerWS.z - lightDirWS.z * dist
	};

	Math::Mat4x4 lightView = Math::Func::MAT4x4::LookAtLH(lightPosWS, centerWS, up);

	// 固定オルソ
	const float half = 50.0f;
	Math::Mat4x4 lightProj = Math::Func::MAT4x4::OrthographicMatrix(
		-half, half,
		half, -half, 
		-150.0f, 150.0f);

	out.cascades[0].view = lightView;
	out.cascades[0].proj = lightProj;
	out.cascades[0].viewProj = lightView * lightProj;

	// splitFar[0] だけ埋める
	out.splitFar[0] = cam.farPlane;
}

void ShadowSystem::SnapOrthoToTexel(Math::Mat4x4& lightView, float orthoWidth, float orthoHeight, uint32_t shadowMapSize)
{
	if (shadowMapSize == 0) return;

	const float texelSizeX = orthoWidth / (float)shadowMapSize;
	const float texelSizeY = orthoHeight / (float)shadowMapSize;
	if (texelSizeX <= 1e-6f || texelSizeY <= 1e-6f) return;

	// light space 原点（WS origin を light space に）
	Math::Vec3f originLS = lightView.TransformPoint({ 0,0,0 });

	originLS.x = std::floor(originLS.x / texelSizeX) * texelSizeX;
	originLS.y = std::floor(originLS.y / texelSizeY) * texelSizeY;

	// 平行移動だけ調整（君の行列規約に合わせてる：m[3][*] が translate）
	lightView.m[3][0] = -(
		originLS.x * lightView.m[0][0] +
		originLS.y * lightView.m[1][0] +
		originLS.z * lightView.m[2][0]
		);
	lightView.m[3][1] = -(
		originLS.x * lightView.m[0][1] +
		originLS.y * lightView.m[1][1] +
		originLS.z * lightView.m[2][1]
		);
	lightView.m[3][2] = -(
		originLS.x * lightView.m[0][2] +
		originLS.y * lightView.m[1][2] +
		originLS.z * lightView.m[2][2]
		);
}

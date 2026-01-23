#include "ShadowSystem.h"
#include "Framework/World/World.h"
#include "Framework/System/Camera/CameraSystem.h"
#include "Framework/Context/CameraContext.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include <algorithm>
#include <cmath>
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

void ShadowSystem::Update(float)
{
	BuildShadowContext();
}

void ShadowSystem::BuildShadowContext()
{
	// 初期化
	context_ = {};
	context_.enabled = false;
	context_.cascadeCount = 0;

	// =====================================================
	// 0) カメラ情報
	// =====================================================
	// ※ API は君の CameraSystem に合わせて差し替え
	auto camSys = world_.GetSystem<CameraSystem>();
	const CameraContext& cam = camSys->GetCameraContext();

	if (!cam.valid)
		return;

	// =====================================================
	// 1) DirectionalLight を 1つ選ぶ
	// =====================================================
	const DirectionalLightComponent* chosenDL = nullptr;
	const TransformComponent* chosenTR = nullptr;

	for (auto [tr, dl] : world_.View<TransformComponent, DirectionalLightComponent>())
	{
		if (!dl.shadow.castShadow) continue;
		if (dl.intensity <= 0.0f) continue;

		chosenDL = &dl;
		chosenTR = &tr;
		break;
	}

	if (!chosenDL || !chosenTR)
		return;

	// ShadowContext の共通項目を埋める
	context_.enabled = true;
	context_.shadowMapSize = chosenDL->shadow.shadowMapSize;

	// =====================================================
	// 2) CSM Split 設定
	// =====================================================
	const uint32_t cascadeCount = 4;
	context_.cascadeCount = cascadeCount;

	// Split の比率
	const float lambda = 0.7f;

	const float n = cam.nearPlane;
	const float f = cam.farPlane;

	float splits[5]{}; // 0..4
	splits[0] = n;
	for (int i = 1; i <= 4; ++i)
	{
		float p = (float)i / 4.0f;
		float logSplit = n * std::pow(f / n, p);
		float uniSplit = n + (f - n) * p;
		splits[i] = lambda * logSplit + (1.0f - lambda) * uniSplit;
	}

	// =====================================================
	// 3) Light View
	// =====================================================
	Math::Vec3f lightDirWS = NormalizeSafe(chosenTR->forward, { 0,-1,0 });

	Math::Vec3f up{ 0,1,0 };
	// up と平行すぎると LookAt が不安定になるので回避
	const float dotUp = lightDirWS.x * up.x + lightDirWS.y * up.y + lightDirWS.z * up.z;
	if (std::abs(dotUp) > 0.99f) up = { 1,0,0 };

	// =====================================================
	// 4) 各カスケードで Ortho を組む
	// =====================================================
	// invViewProj を使って frustum corner を world に戻す
	const Math::Mat4x4 invViewProj = cam.viewProj.Inverse();

	for (uint32_t ci = 0; ci < cascadeCount; ++ci)
	{
		const float cn = splits[ci];
		const float cf = splits[ci + 1];

		// ---- カスケード用プロジェクションを作り直す ----
		Math::Mat4x4 cascadeProj = Math::Func::MAT4x4::PerspectiveFovMatrix(
			cam.fovY, cam.aspectRatio, cn, cf
		);

		Math::Mat4x4 cascadeViewProj = cam.view * cascadeProj;
		Math::Mat4x4 invCascadeVP = cascadeViewProj.Inverse();

		// frustum corners in WS
		Math::Vec3f cornersWS[8]{};
		GetFrustumCornersWS(invCascadeVP, 0.0f, 1.0f, cornersWS);

		// カスケード中心
		Math::Vec3f centerWS = Average8(cornersWS);

		// 仮想ライト位置（中心を照らす）
		const float dist = (cf - cn) + chosenDL->shadow.orthoHalfSize;
		Math::Vec3f lightPosWS = {
			centerWS.x - lightDirWS.x * dist,
			centerWS.y - lightDirWS.y * dist,
			centerWS.z - lightDirWS.z * dist
		};

		Math::Mat4x4 lightView = Math::Func::MAT4x4::LookAtLH(
			lightPosWS,
			centerWS,
			up
		);

		// corners を light space へ
		Math::Vec3f minLS{ +1e30f, +1e30f, +1e30f };
		Math::Vec3f maxLS{ -1e30f, -1e30f, -1e30f };

		for (int i = 0; i < 8; ++i)
		{
			Math::Vec3f pLS = lightView.TransformPoint(cornersWS[i]);
			minLS.x = std::min(minLS.x, pLS.x);
			minLS.y = std::min(minLS.y, pLS.y);
			minLS.z = std::min(minLS.z, pLS.z);
			maxLS.x = std::max(maxLS.x, pLS.x);
			maxLS.y = std::max(maxLS.y, pLS.y);
			maxLS.z = std::max(maxLS.z, pLS.z);
		}

		// 余白（シャドウの泳ぎ・クリップ回避）
		const float padXY = 5.0f;
		const float padZ = 20.0f;
		minLS.x -= padXY; minLS.y -= padXY;
		maxLS.x += padXY; maxLS.y += padXY;
		minLS.z -= padZ;
		maxLS.z += padZ;

		Math::Mat4x4 lightProj = Math::Func::MAT4x4::OrthographicMatrix(
			minLS.x, maxLS.x,
			minLS.y, maxLS.y,
			minLS.z, maxLS.z
		);

		ShadowCascade& c = context_.cascades[ci];
		c.view = lightView;
		c.proj = lightProj;
		c.viewProj = lightView * lightProj;
	}
}
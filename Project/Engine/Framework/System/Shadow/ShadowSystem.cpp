#include "../../../Math/TMath.h"
#include "ShadowSystem.h"
#include "ShadowHelpers.h"
#include "Framework/World/World.h"
#include "Framework/System/Camera/CameraSystem.h"
#include "Framework/Context/CameraContext.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Light/SpotLightComponent.h"
#include "Framework/Component/Shadow/ShadowComponent.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#undef min
#undef max

using namespace Tsumi;
using namespace Framework;

ShadowSystem::ShadowSystem(World& world)
	: ISystem(world)
{
}

void ShadowSystem::Init()
{
	BuildDefault(defaultCtx_);
	activeCtx_ = defaultCtx_;
}

void ShadowSystem::Update(float)
{
	ShadowContext ctx{};

	// 有効なDirectionalLightが見つかったか？
	bool foundCaster = BuildShadowContext(ctx);

	// SpotLightの影構築
	BuildSpotShadowContext(ctx);

	if (foundCaster || ctx.spotEnabled)
	{
		activeCtx_ = ctx;
	}
	else
	{
		// どちらも無い場合
		BuildDefault(activeCtx_);
		activeCtx_.enabled = false;
		activeCtx_.spotEnabled = false;
	}
}

bool ShadowSystem::BuildShadowContext(ShadowContext& out)
{
	out = {};
	out.enabled = false;

	// Camera
	auto camSys = world_.GetSystem<CameraSystem>();
	const CameraContext& cam = camSys->GetContext();
	if (!cam.valid)
	{
		return false;
	}

	// Choose 1 directional light
	// Choose 1 directional light
	const DirectionalLightComponent* chosenDL = nullptr;
	const ShadowComponent* chosenShadow = nullptr;
	const TransformComponent* chosenTR = nullptr;
	uint32_t shadowMapSize = 2048; // Default

	// Iterate all Directional Lights
	for (auto [tr, dl] : world_.View<TransformComponent, DirectionalLightComponent>())
	{
		if (dl.intensity <= 0.0f) continue;
		if (!dl.castShadow) continue; // Check Light's flag

		// Try to find ShadowComponent
		ShadowComponent* sh = nullptr;
		if (auto* owner = dl.GetOwner()) {
			sh = owner->GetComponent<ShadowComponent>();
		}

		// If ShadowComponent exists, check its castShadow too?
		if (sh) {
			if (!sh->castShadow) continue;
			// Use ShadowComponent's size if valid
			if (sh->shadowMapSize > 0) {
				shadowMapSize = sh->shadowMapSize;
			}
		}

		chosenDL = &dl;
		chosenShadow = sh; // Can be null
		chosenTR = &tr;
		break; // Only 1 directional light supported
	}

	if (!chosenDL || !chosenTR)
	{
		// No caster
		return false;
	}

	out.enabled = true;
	out.shadowMapSize = shadowMapSize;

	// CSM splits
	constexpr uint32_t cascadeCount = 4;
	out.cascadeCount = cascadeCount;

	// Lambda controls the split distribution (0 = uniform, 1 = logarithmic)
	// Reduced to 0.6 to give Cascade 0 more range and prevent it from being too small/useless.
	const float lambda = 0.6f;
	const float n = cam.nearPlane;
	// Use Light's FarZ as Shadow Distance, clamped by Camera Far
	const float f = std::min(cam.farPlane, chosenDL->farZ);

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
		// 実際にカメラのFOVやNear/Farから計算した splits[i + 1] を渡す
		out.splitFar[i] = splits[i + 1];
	}

	// Light dir
	Math::Vec3f lightDirWS = ShadowDetail::ShadowMath::NormalizeSafe(chosenTR->forward, { 0,-1,0 });

	Math::Vec3f up{ 0,1,0 };
	const float dotUp = lightDirWS.x * up.x + lightDirWS.y * up.y + lightDirWS.z * up.z;
	if (std::abs(dotUp) > 0.99f) up = { 1,0,0 };

	for (uint32_t ci = 0; ci < cascadeCount; ++ci)
	{
		const float cn = splits[ci];
		const float cf = splits[ci + 1];

		const Math::Mat4x4 cascadeProj =
			Math::Func::MAT4x4::PerspectiveFovMatrix(cam.fovY, cam.aspectRatio, cn, cf);

		const Math::Mat4x4 cascadeVP = cam.view * cascadeProj;
		const Math::Mat4x4 invCascadeVP = cascadeVP.Inverse();

		// ===== Bounding Sphere (View Space Calculation for Stability) =====
		Math::Vec3f cornersVS[8]{};
		ShadowDetail::ShadowMath::GetFrustumCornersWS(cascadeProj.Inverse(), 0.0f, 1.0f, cornersVS); // Using proj inverse gives View Space corners

		Math::Vec3f centerVS = ShadowDetail::ShadowMath::Average8(cornersVS);
		
		float radius = 0.0f;
		for (const auto& c : cornersVS) {
			float d = (c - centerVS).Length();
			radius = std::max(radius, d);
		}

		radius = std::max(radius, 50.0f);
		
		radius = std::ceil(radius);

		// Transform center to World Space
		Math::Mat4x4 invView = cam.view.Inverse();
		Math::Vec3f sphereCenterWS = invView.TransformPoint(centerVS);

		// Light Orientation
		Math::Mat4x4 lightViewRot = Math::Func::MAT4x4::LookAtLH({ 0,0,0 }, lightDirWS, up);
		
		Math::Vec3f centerLS = lightViewRot.TransformPoint(sphereCenterWS);

		// ===== Texel Snap =====
		float texelSize = (radius * 2.0f) / (float)out.shadowMapSize;

		if (texelSize > 0.0001f) {
			centerLS.x = std::floor(centerLS.x / texelSize) * texelSize;
			centerLS.y = std::floor(centerLS.y / texelSize) * texelSize;
		}

		Math::Mat4x4 lightViewRotInv = lightViewRot.Transpose(); 
		Math::Vec3f snappedCenterWS = lightViewRotInv.TransformPoint(centerLS);

		// ===== Final Light View Matrix =====
		const float backDistance = 500.0f;
		const float dist = radius + backDistance;

		const Math::Vec3f lightPosWS = {
			snappedCenterWS.x - lightDirWS.x * dist,
			snappedCenterWS.y - lightDirWS.y * dist,
			snappedCenterWS.z - lightDirWS.z * dist
		};

		Math::Mat4x4 lightView = Math::Func::MAT4x4::LookAtLH(lightPosWS, snappedCenterWS, up);


		// ===== 固定サイズ Ortho =====
		float r = radius;
		float nearZ = dist - r; 
		float farZ = dist + r;

		const Math::Mat4x4 lightProj = Math::Func::MAT4x4::OrthographicMatrix(
			-r,     // left
			r,      // right
			-r,     // bottom
			r,      // top
			nearZ,  // nearClip
			farZ    // farClip
		);

		auto& c = out.cascades[ci];
		c.view = lightView;
		c.proj = lightProj;
		c.viewProj = lightView * lightProj;
	}

	return true;
}

void ShadowSystem::BuildSpotShadowContext(ShadowContext& out)
{
	out.spotEnabled = false;
	out.spotShadowMapSize = 1024; // Default
	out.activeSpotShadowCount = 0;

	// 1. Collect Spot Lights
	struct LightEntry {
		const TransformComponent* tr;
		const SpotLightComponent* sl;
		ShadowComponent* sh; // Can be nullptr
		uint32_t mapSize;
	};
	std::vector<LightEntry> lights;
	lights.reserve(ShadowContext::kMaxSpotShadows);

	// Iterate all SpotLights
	for (auto [tr, sl] : world_.View<TransformComponent, SpotLightComponent>())
	{
		ShadowComponent* sh = nullptr;

		if (!sl.castShadow) continue;
		if (sl.intensity <= 0.0f) continue;

		auto* owner = sl.GetOwner();
		if (owner) {
			sh = owner->GetComponent<ShadowComponent>();
		}

		uint32_t size = 1024;
		if (sh) {
			if (!sh->castShadow) continue;
			size = sh->shadowMapSize;
		}

		lights.push_back({ &tr, &sl, sh, size });
	}

	if (lights.empty()) return;

	// Sort? (Optional)

	const uint32_t count = std::min((uint32_t)lights.size(), ShadowContext::kMaxSpotShadows);

	for (uint32_t i = 0; i < count; ++i)
	{
		auto& entry = lights[i];

		if (entry.sh) {
			entry.sh->spotShadowIndex = (int32_t)i;
		}
		auto* slMutable = const_cast<SpotLightComponent*>(entry.sl);
		slMutable->runtimeShadowIndex = (int32_t)i;

		auto& data = out.spotShadows[i];
		data.lightIndex = 0;

		Math::Vec3f pos = { entry.tr->world.m[3][0], entry.tr->world.m[3][1], entry.tr->world.m[3][2] };
		Math::Vec3f fwd = entry.tr->forward;
		Math::Vec3f up = entry.tr->up;

		Math::Mat4x4 view = Math::Func::MAT4x4::LookAtLH(pos, pos + fwd, up);

		float angleDeg = std::min(entry.sl->outerAngle * 2.0f, 179.0f);
		float angle = Math::Func::NUM::ToRadians(angleDeg);

		// Use component params or fallback
		float n = entry.sl->nearZ;
		float f = entry.sl->farZ;
		if (entry.sh) {
			n = entry.sh->nearZ;
			f = entry.sh->farZ;
		}

		Math::Mat4x4 proj = Math::Func::MAT4x4::PerspectiveFovMatrix(angle, 1.0f, n, f);

		data.viewProj = view * proj;
	}

	out.activeSpotShadowCount = count;
	out.spotEnabled = (count > 0);
}

void ShadowSystem::BuildDefault(ShadowContext& out)
{
	out = {};
	out.enabled = true;

	out.shadowMapSize = 1024;
	out.cascadeCount = 4;

	auto camSys = world_.GetSystem<CameraSystem>();
	const CameraContext& cam = camSys->GetContext();

	// 左上前（-1, 1, -1）あたりから原点を見下ろすようなライト方向にする
	// ライトの方向ベクトルなので、光源からターゲットへの向き
	Math::Vec3f lightDirWS = ShadowDetail::ShadowMath::NormalizeSafe({ 1.0f, -1.0f, 1.0f }, { 0,-1,0 });

	Math::Vec3f up{ 0,1,0 };
	const float dotUp = lightDirWS.x * up.x + lightDirWS.y * up.y + lightDirWS.z * up.z;
	if (std::abs(dotUp) > 0.99f) up = { 1,0,0 };

	const float lambda = 0.96f;
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

	for (uint32_t ci = 0; ci < 4; ++ci)
	{
		//float cn = splits[ci];
		float cf = splits[ci + 1];

		// カメラに関係なく原点 (0,0,0) を見る
		const Math::Vec3f centerWS = { 0.0f, 0.0f, 0.0f };
		const float dist = 50.0f;
		const Math::Vec3f lightPosWS = {
			centerWS.x - lightDirWS.x * dist,
			centerWS.y - lightDirWS.y * dist,
			centerWS.z - lightDirWS.z * dist
		};

		Math::Mat4x4 lightView = Math::Func::MAT4x4::LookAtLH(lightPosWS, centerWS, up);

		const float half = 150.0f;
		Math::Mat4x4 lightProj = Math::Func::MAT4x4::OrthographicMatrix(
			-half,   // left
			half,    // right
			-half,   // bottom
			half,    // top
			-150.0f, // nearClip
			150.0f   // farClip
		);

		out.cascades[ci].view = lightView;
		out.cascades[ci].proj = lightProj;
		out.cascades[ci].viewProj = lightView * lightProj;

		out.splitFar[ci] = cf;
	}
}

void ShadowSystem::SnapOrthoToTexel(Math::Mat4x4& lightView, float orthoWidth, float orthoHeight, uint32_t shadowMapSize)
{
	if (shadowMapSize == 0) return;

	const float texelSizeX = orthoWidth / (float)shadowMapSize;
	const float texelSizeY = orthoHeight / (float)shadowMapSize;
	if (texelSizeX <= 1e-6f || texelSizeY <= 1e-6f) return;

	// light space 原点（WS origin を light space に）
	// View Matrix (LookAt) の Translation 部分 (m[3]) は "World Origin in View Space" を表す
	// したがって、この値をTexelサイズでSnapすれば、World GridとTexel Gridが整合する

	lightView.m[3][0] = std::floor(lightView.m[3][0] / texelSizeX) * texelSizeX;
	lightView.m[3][1] = std::floor(lightView.m[3][1] / texelSizeY) * texelSizeY;
}

void ShadowSystem::CalculateBoundingSphere(const Math::Mat4x4& invViewProj, float nearZ, float farZ, Math::Vec3f& outCenter, float& outRadius)
{
	Math::Vec3f corners[8];
	ShadowDetail::ShadowMath::GetFrustumCornersWS(invViewProj, nearZ, farZ, corners);

	// center = 平均
	outCenter = ShadowDetail::ShadowMath::Average8(corners);

	// 半径 = 最大距離
	outRadius = 0.0f;
	for (int i = 0; i < 8; ++i)
	{
		Math::Vec3f d = {
			corners[i].x - outCenter.x,
			corners[i].y - outCenter.y,
			corners[i].z - outCenter.z
		};

		float dist = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
		outRadius = std::max(outRadius, dist);
	}
}

#include "../../../Math/TMath.h"
#include "ShadowSystem.h"
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
	
	// 有効なDirectionalLightが見つかったか？
	bool foundCaster = BuildShadowContext(ctx);
	
	// SpotLightの影構築
	BuildSpotShadowContext(ctx);

	if (foundCaster || ctx.spotEnabled)
	{
		// どちらか片方でも有効なら採用
		activeCtx_ = ctx;
		// ただし foundCaster が false なら enabled=false になるので、
		// Spotだけ有効なケースを考慮して enabled フラグの扱いを調整する必要がある。
		// ShadowContext.enabled は "CSM Enabled" の意味合いが強かった。
		// SpotEnabled は別フラグ (spotEnabled) にしたので、混在しても大丈夫。
		// activeCtx_.enabled = foundCaster; // CSM有効フラグとして使う
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

	const float lambda = 0.96f;
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
	// FORCE DEBUG DIRECTION: Down
	Math::Vec3f lightDirWS = NormalizeSafe({ 0.1f, -1.0f, 0.1f }, { 0,-1,0 });

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

		Math::Vec3f cornersWS[8]{};
		GetFrustumCornersWS(invCascadeVP, 0.0f, 1.0f, cornersWS);

		const Math::Vec3f centerWS = Average8(cornersWS);

		// 2. Build Light View Matrix (Standard "Look At Center")
		// Place the camera exactly at the center (virtual) but look from light direction
		Math::Vec3f eyeWS = centerWS - lightDirWS * 500.0f; // Pull back 500m
		Math::Mat4x4 lightView = Math::Func::MAT4x4::LookAtLH(eyeWS, centerWS, up);

		// 3. Build Fixed Ortho Projection (Debug Simplification)
		// FORCE HUGE SIZE
		float boxSize = 500.0f; 

		float radius = boxSize;
		Math::Vec3f centerLS = lightView.TransformPoint(centerWS);
		
		float left = centerLS.x - radius;
		float right = centerLS.x + radius;
		float bottom = centerLS.y - radius;
		float top = centerLS.y + radius;
		
		float zn = centerLS.z - 1000.0f; 
		float zf = centerLS.z + 1000.0f;

		const Math::Mat4x4 lightProj = Math::Func::MAT4x4::OrthographicMatrix(
			left, top, right, bottom, zn, zf
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
	// Transform, SpotLight (ShadowComponent is optional now)
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
		// Check explicit ShadowComponent
		ShadowComponent* sh = nullptr;
		// Since we don't have GetComp<T>(entity) easily in this View loop unless we have entity ID,
		// we might need a different approach or just iterate separately.
		// However, World::View usually gives components. 
		// Let's assume we can't easily get sibling component without Entity ID.
		// BUT, we can iterate <Transform, Spot, Shadow> and <Transform, Spot> separately? No that's duplicate.
		// Standard ECS approach: Iterate Entity that has Transform & Spot. Then TryGet Shadow.
		// If View definition supports Entity ID, use that. 
		// Current codebase View seems to return tuples of references/pointers.
		
		// Workaround: We iterate all SpotLights. 
		// If the engine ECS doesn't support "Optional" component in View, we might need a helper.
		// For now, let's assume we iterate all SpotLights, and we need to find if they have ShadowComponent.
		// If we can't do that easily, we might need to rely on the user adding ShadowComponent for *advanced* features,
		// but for *basic* features we rely on SpotLightComponent's new fields.
		
		// Wait, if we want to support "SpotLight without ShadowComponent casts shadow", 
		// we need to know if it *should* cast shadow.
		// SpotLightComponent now has `castShadow`.
		
		if (!sl.castShadow) continue;
		if (sl.intensity <= 0.0f) continue;

		// Try to find ShadowComponent on this entity?
		// Since I don't see EntityID in the loop `auto [tr, sl]`, I cannot query ShadowComponent easily.
		// I will rely on `SpotLightComponent`'s params for now.
		// If ShadowComponent exists, it would be great to use it.
		// Let's check `ShadowSystem.cpp` imports. It includes `World.h`.
		// If `View` returns valid pointers, maybe I can find owner?
		// `IComponent` has `GetOwner()`.
		
		auto* owner = sl.GetOwner();
		if (owner) {
			sh = owner->GetComponent<ShadowComponent>();
		}

		uint32_t size = 1024;
		if (sh) {
			// If ShadowComponent exists, we might defer to its castShadow flag? 
			// Or we sync them?
			// Let's say: If ShadowComponent is present, use its settings.
			// But SpotLightComponent also has castShadow. Confusion might arise.
			// DESIGN DECISION: SpotLightComponent.castShadow is the master switch for "Simple usage".
			// If ShadowComponent is attached, we override size/bias etc.
			// But if ShadowComponent.castShadow is false, should we disable it?
			// Let's assume yes.
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
		// Also set on SpotLightComponent so LightSystem can pick it up without looking for ShadowComponent
		auto* slMutable = const_cast<SpotLightComponent*>(entry.sl);
		slMutable->runtimeShadowIndex = (int32_t)i; 
		
		auto& data = out.spotShadows[i];
		data.lightIndex = 0;

		Math::Vec3f pos = { entry.tr->world.m[3][0], entry.tr->world.m[3][1], entry.tr->world.m[3][2] };
		Math::Vec3f fwd = entry.tr->forward; 
		Math::Vec3f up = entry.tr->up;

		Math::Mat4x4 view = Math::Func::MAT4x4::LookAtLH(pos, pos + fwd, up);

		// Use outerAngle directly to ensure valid FOV, even if LightSystem hasn't updated outerCos yet.
		// Clamp angle to avoid 180 deg (Pi) which breaks perspective projection.
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
	out.enabled = true; // Force enable for debugging if needed, or false.
	// Let's keep it enabled so we can see *something* if the system falls back.
	
	out.shadowMapSize = 1024;
	out.cascadeCount = 4;

	// Use a fixed light direction (Down-Forward-Right)
	Math::Vec3f lightDir = NormalizeSafe({ 1.0f, -1.0f, 1.0f }, { 0,-1,0 });
	Math::Vec3f up = { 0,1,0 };
	if (std::abs(lightDir.y) > 0.99f) up = { 1,0,0 };

	// Fixed View: Look at Origin from 50m away
	Math::Vec3f centerS = { 0,0,0 };
	Math::Vec3f eyeS = centerS - lightDir * 50.0f;
	Math::Mat4x4 view = Math::Func::MAT4x4::LookAtLH(eyeS, centerS, up);

	// Fixed Proj: Ortho 20x20 area
	// This ensures that if the camera is near origin, we see shadows.
	Math::Mat4x4 proj = Math::Func::MAT4x4::OrthographicMatrix(-20, 20, 20, -20, 0.1f, 200.0f);

	for (int i = 0; i < 4; ++i) {
		out.cascades[i].view = view;
		out.cascades[i].proj = proj;
		out.cascades[i].viewProj = view * proj;
		out.splitFar[i] = (float)(i + 1) * 10.0f; // Dummy splits
	}
}

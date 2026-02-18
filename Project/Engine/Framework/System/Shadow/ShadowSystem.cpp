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

	// 1. Find the primary directional light
	// (For now, just pick the first one that casts shadows or has high intensity)
	const DirectionalLightComponent* chosenDL = nullptr;
	const ShadowComponent* chosenShadow = nullptr;
	const TransformComponent* chosenTR = nullptr;

	for (auto [tr, dl] : world_.View<TransformComponent, DirectionalLightComponent>())
	{
		if (dl.intensity <= 0.0f) continue;
		
		// Optional: Check ShadowComponent on the same entity
		ShadowComponent* sh = nullptr;
		auto* owner = dl.GetOwner();
		if (owner) {
			sh = owner->GetComponent<ShadowComponent>();
		}

		if (!dl.castShadow) {
			// If light doesn't want shadow, skip for SHADOW generation, 
			// but we might still want it as a light.
			// context logic implies this builds shadow data.
			continue;
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

	// 2. Prepare Config
	// Use ShadowComponent settings if available, else defaults
	ShadowDetail::ShadowCascadeConfig config;
	config.cascadeCount = 4;
	config.shadowMapSize = (float)2048; // Default
	config.lambda = 0.96f;
	config.lightNearZ = chosenDL->nearZ;
	config.lightFarZ = chosenDL->farZ;
	config.lightOrthoHalfSize = chosenDL->orthoHalfSize;

	if (chosenShadow) {
		// If we had per-component settings, applied here.
		// For now, ShadowComponent might just be a marker or hold basic info.
		if (chosenShadow->shadowMapSize > 0) {
			config.shadowMapSize = (float)chosenShadow->shadowMapSize;
		}
		config.lightNearZ = chosenShadow->nearZ;
		config.lightFarZ = chosenShadow->farZ;
		config.lightOrthoHalfSize = chosenShadow->orthoHalfSize;
	}

	// 3. Get Camera
	// Already have 'cam' from the beginning of the function.

	// 4. Run Generator
	// Light dir: Forward vector of the light transform
	// Ensure normalized
	Math::Vec3f lightDir = ShadowDetail::ShadowMath::NormalizeSafe(chosenTR->forward, { 0, -1, 0 });

	ShadowDetail::CSMGenerator generator;
	generator.Update(cam, lightDir, config, out);
	
	// Copy extra info? 
	// generator.Update fills 'out' (cascades, splitFar, etc.)
	
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
	out.enabled = true; // Force enable for debugging if needed, or false.
	// Let's keep it enabled so we can see *something* if the system falls back.
	
	out.shadowMapSize = 1024;
	out.cascadeCount = 4;

	// Use a fixed light direction (Down-Forward-Right)
	Math::Vec3f lightDir = NormalizeSafe({ 1.0f, -1.0f, 1.0f }, { 0,-1,0 });
	Math::Vec3f up = { 0,1,0 };
	if (std::abs(lightDir.y) > 0.99f) up = { 1,0,0 };

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
			-half,  // left
			 half,  // top
			 half,  // right
			-half,  // bottom
			-150.0f, 150.0f);

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

#include "ShadowSystem.h"
#include "../../../Math/TMath.h"
#include "Framework/World/World.h"
#include "Framework/System/Camera/CameraSystem.h"
#include "Framework/Context/CameraContext.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Light/SpotLightComponent.h"
#include "Framework/Component/Shadow/ShadowComponent.h"
#include "ShadowHelpers.h"
#include <algorithm>
#include <cmath>
#include <cassert>
#undef min
#undef max

using namespace Tsumi;
using namespace Framework;
using namespace Framework::ShadowDetail;


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
	// 1. Find the primary directional light
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
	config.lightOrthoHalfSize = chosenDL->orthoHalfSize; // Used for padding if needed

	if (chosenShadow) {
		if (chosenShadow->shadowMapSize > 0) {
			config.shadowMapSize = (float)chosenShadow->shadowMapSize;
		}
		// Overwrite with ShadowComponent specific settings if desired,
		// though typically DL settings control the light volume.
	}

	// 3. Get Camera
	auto* camSys = world_.GetSystem<CameraSystem>();
	if (!camSys) return false;
	const auto& cam = camSys->GetContext();

	// 4. Run Generator
	// Light dir: Forward vector of the light transform
	// Ensure normalized. Assuming 'forward' is correct, but re-normalizing is safe.
	Math::Vec3f lightDir = ShadowDetail::ShadowMath::NormalizeSafe(chosenTR->forward, { 0, -1, 0 });

	ShadowDetail::CSMGenerator generator;
	generator.Update(cam, lightDir, config, out);
	
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

		uint32_t size = 512;
		if (sh && sh->shadowMapSize > 0) size = sh->shadowMapSize;

		lights.push_back({ &tr, &sl, sh, size });
	}

	if (lights.empty()) {
		out.spotEnabled = false;
		return;
	}

	// Limit count
	if (lights.size() > ShadowContext::kMaxSpotShadows) {
		lights.resize(ShadowContext::kMaxSpotShadows);
	}

	out.spotEnabled = true;
	out.activeSpotShadowCount = (uint32_t)lights.size();
	
	// Use max size among lights or constant? Let's pick max.
	uint32_t maxSz = 512;
	for (auto& l : lights) maxSz = std::max(maxSz, l.mapSize);
	out.spotShadowMapSize = maxSz;

	for (size_t i = 0; i < lights.size(); ++i)
	{
		auto& entry = lights[i];
		auto& info = out.spotShadows[i];

		Math::Vec3f pos = entry.tr->GetWorldPos();
		Math::Vec3f dir = ShadowDetail::ShadowMath::NormalizeSafe(entry.tr->forward, {0,0,1}); // Spot direction
		Math::Vec3f up = { 0,1,0 };
		
		// Spot Light View
		// LookAtLH: eye, focus, up.
		// focus = pos + dir
		Math::Vec3f focus = { pos.x + dir.x, pos.y + dir.y, pos.z + dir.z };
		info.view = Math::Func::MAT4x4::LookAtLH(pos, focus, up);

		// Spot Light Proj
		// PerspectiveFov
		// SpotLightComponent has innerAngle/outerAngle in Degrees (based on usage elsewhere typical).
		// standard spot light "angle" usually refers to the half-angle of the cone.
		// PerspectiveFov takes fovY (Full vertical angle) in Radians.
		// So we need outerAngle * 2 * Deg2Rad.
		
		float halfAngleDeg = entry.sl->outerAngle > 0.0f ? entry.sl->outerAngle : 45.0f;
		float fovRad = (halfAngleDeg * 2.0f) * (3.14159f / 180.0f);
		
		float aspect = 1.0f; // Shadow map is square
		float nz = entry.sl->nearZ > 0.0f ? entry.sl->nearZ : 0.1f;
		float fz = entry.sl->farZ > 0.0f ? entry.sl->farZ : 100.0f; 
		// Use component's nearZ/farZ, fallbacks if needed.
		// SpotLightComponent has nearZ/farZ fields.

		info.proj = Math::Func::MAT4x4::PerspectiveFovMatrix(fovRad, aspect, nz, fz);
		info.viewProj = info.view * info.proj;
	}
}

void ShadowSystem::BuildDefault(ShadowContext& out)
{
	out = {};
	out.enabled = true; // Debug default
	
	out.shadowMapSize = 1024;
	out.cascadeCount = 4;

	// Use a fixed light direction (Down-Forward-Right)
	Math::Vec3f lightDir = ShadowDetail::ShadowMath::NormalizeSafe({ 1.0f, -1.0f, 1.0f }, { 0,-1,0 });
	Math::Vec3f up = { 0,1,0 };
	if (std::abs(lightDir.y) > 0.99f) up = { 1,0,0 };

	// Fixed View
	Math::Vec3f centerS = { 0,0,0 };
	Math::Vec3f eyeS = centerS - lightDir * 50.0f;
	Math::Mat4x4 view = Math::Func::MAT4x4::LookAtLH(eyeS, centerS, up);

	// Fixed Proj
	Math::Mat4x4 proj = Math::Func::MAT4x4::OrthographicMatrix(-20, 20, 20, -20, 0.1f, 200.0f);

	for (int i = 0; i < 4; ++i) {
		out.cascades[i].view = view;
		out.cascades[i].proj = proj;
		out.cascades[i].viewProj = view * proj;
		out.splitFar[i] = (float)(i + 1) * 10.0f; 
	}
}

void ShadowSystem::SnapOrthoToTexel(Math::Mat4x4&, float, float, uint32_t)
{
	// Deprecated / Unused: Logic moved to CSMGenerator
}

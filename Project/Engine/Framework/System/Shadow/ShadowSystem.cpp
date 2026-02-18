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
	Math::Vec3f lightDirWS = NormalizeSafe(chosenTR->forward, { 0,-1,0 });

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

		float orthoSize = chosenDL->orthoHalfSize;
		if (chosenShadow) {
			// If ShadowComponent defines orthoHalfSize, use it?
			// Assumption: ShadowComponent also has this field.
			// Let's defer to ShadowComponent if it looks customized (e.g. not default).
			// But for simplicity, let's say DirectionalLightComponent is the source of truth for projection size now.
			// Or we check which one is "touched".
			// Given the refactor goal is to move props to Light, let's prefer Light's prop.
			// But ShadowComponent might be used for "Advanced Override".
			// Let's stick to Light Component for now as requested.
		}

		const float dist = (cf - cn) + orthoSize;
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
		const float padXY = chosenDL->orthoHalfSize;  // Use component ortho size for XY padding
		const float padZ = chosenDL->nearZ;           // Use component nearZ for back coverage distance
		minLS.x -= padXY; minLS.y -= padXY;
		maxLS.x += padXY; maxLS.y += padXY;
		minLS.z -= padZ;
		maxLS.z += padZ;

		// Use Light's FarZ as Shadow Distance (CSM Max Distance)
		// We clamp the last cascade's far plane with chosenDL->farZ earlier (in splits calculation loop).
		// Here we just ensure the ortho projection covers the casters.
		
		// Ah, I need to update the split calculation logic earlier in the function!
		// But I am restricted to replace this chunk.
		// Standard CSM implementation uses minLS.z/maxLS.z for Z range.
		// So `nearZ`/`farZ` of light component might be better interpreted as "Shadow Max Distance".
		
		const Math::Mat4x4 lightProj = Math::Func::MAT4x4::OrthographicMatrix(
			minLS.x, // left
			maxLS.y, // top
			maxLS.x, // right
			minLS.y, // bottom
			minLS.z, // nearZ
			maxLS.z  // farZ
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
	out.enabled = true;

	out.shadowMapSize = 1024;
	out.cascadeCount = 4;

	auto camSys = world_.GetSystem<CameraSystem>();
	const CameraContext& cam = camSys->GetContext();

	// 左上前（-1, 1, -1）あたりから原点を見下ろすようなライト方向にする
	// ライトの方向ベクトルなので、光源からターゲットへの向き
	Math::Vec3f lightDirWS = NormalizeSafe({ 1.0f, -1.0f, 1.0f }, { 0,-1,0 });

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

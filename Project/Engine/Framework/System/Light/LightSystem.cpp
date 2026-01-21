#include "LightSystem.h"
#include "Framework/World/World.h"
#include "Framework/System/Camera/CameraSystem.h"
#include "Framework/Context/CameraContext.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"

using namespace Tsumi;
using namespace Tsumi::Framework;

LightSystem::LightSystem(World& world)
	: world_(world)
{}

void LightSystem::Update(float)
{
	BuildMainDirectional();
}

void LightSystem::BuildMainDirectional()
{
	//mainDir_ = {};
	//mainDir_.valid = false;

	//DirectionalLightComponent* selectedLight = nullptr;
	//TransformComponent* selectedTr = nullptr;

	//DirectionalLightComponent* fallbackLight = nullptr;
	//TransformComponent* fallbackTr = nullptr;

	//// ---- 1) DirectionalLight を列挙 ----
	//for (auto&& [light, tr] :
	//	 world_.View<DirectionalLightComponent, TransformComponent>())
	//{
	//	if (light.castShadow)
	//	{
	//		selectedLight = &light;
	//		selectedTr = &tr;
	//		break;
	//	}

	//	if (!fallbackLight)
	//	{
	//		fallbackLight = &light;
	//		fallbackTr = &tr;
	//	}
	//}

	//// fallback 適用
	//if (!selectedLight)
	//{
	//	selectedLight = fallbackLight;
	//	selectedTr = fallbackTr;
	//}

	//if (!selectedLight || !selectedTr)
	//	return;

	//auto* light = selectedLight;
	//auto* tr = selectedTr;

	//// ---- 2) ライト基本パラメータ ----
	//mainDir_.direction = SafeNormalize(light->direction);
	//mainDir_.color = light->color;
	//mainDir_.intensity = light->intensity;

	//mainDir_.shadowResolution = light->shadow.resolution;
	//mainDir_.nearZ = light->shadow.nearZ;
	//mainDir_.farZ = light->shadow.farZ;
	//mainDir_.orthoSize = light->shadow.orthoSize;

	//// ---- 3) focus（CameraSystem と同じ思想）----
	//Tsumi::Math::Vec3f focus{ 0,0,0 };

	//if (auto* camSys = world_.GetSystem<CameraSystem>())
	//{
	//	const auto& cam = camSys->GetCameraContext();
	//	if (cam.valid)
	//		focus = cam.position;
	//}
	//else
	//{
	//	focus = tr->GetWorldPos();
	//}

	//// ---- 4) ライト“カメラ”の Transform を構築 ----
	//// Directional Light = 回転 + 位置を持つ「カメラ」

	//// scale は常に 1
	//Math::Vec3f scale{ 1,1,1 };

	//// 回転：direction を向く回転
	//// ※ TransformComponent を使うなら、ここは tr->srt.rotate でもOK
	//Math::Vec3f rotate = tr->srt.rotate;

	//// 位置：focus から光の逆方向に引く
	//const float dist = light->shadow.cameraDistance;
	//Math::Vec3f translate = {
	//	focus.x - mainDir_.direction.x * dist,
	//	focus.y - mainDir_.direction.y * dist,
	//	focus.z - mainDir_.direction.z * dist,
	//};

	//mainDir_.position = translate;

	//// ---- 5) CameraSystem と同じ作り方で View 行列 ----
	//Math::Mat4x4 lightWorld =
	//	Math::Func::MAT4x4::AffineMatrix(scale, rotate, translate);

	//mainDir_.view = lightWorld.Inverse();

	//// ---- 6) Ortho Projection（Directional 専用）----
	//const float size = mainDir_.orthoSize;

	//mainDir_.proj =
	//	Math::Func::MAT4x4::OrthoLH(
	//	size * 2.0f,
	//	size * 2.0f,
	//	mainDir_.nearZ,
	//	mainDir_.farZ
	//	);

	//// ---- 7) viewProj（CameraSystem と同一規約）----
	//mainDir_.viewProj = mainDir_.view * mainDir_.proj;
	//mainDir_.valid = true;
}

Math::Vec3f LightSystem::SafeNormalize(const Math::Vec3f& v)
{
	const float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
	if (len2 < 1e-8f)
		return { 0.0f, -1.0f, 0.0f };

	const float inv = 1.0f / std::sqrt(len2);
	return { v.x * inv, v.y * inv, v.z * inv };
}

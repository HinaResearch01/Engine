#include "Framework/Actor/IActor.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Light/PointLightComponent.h"
#include "Framework/Component/Light/SpotLightComponent.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Shadow/ShadowComponent.h"
#include "Framework/Component/Render/RenderComponent.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "imgui.h"

namespace Tsumi::Framework {

void TransformComponent::OnInspectorGui()
{
	if (ImGui::TreeNode("Transform")) {
		bool isChanged = false;
		if (ImGui::DragFloat3("Scale", &srt.scale.x, 0.01f, 0.01f, 1000000.0f)) isChanged = true;
		if (ImGui::DragFloat3("Rotation", &srt.rotate.x, 0.1f)) isChanged = true;
		if (ImGui::DragFloat3("Translate", &srt.translate.x, 0.1f)) isChanged = true;
		
		// Dirty flag set
		if (isChanged) {
			MarkDirty();
		}

		ImGui::TreePop();
	}
}

void CameraComponent::OnInspectorGui()
{
	if (ImGui::TreeNode("Camera")) {
		ImGui::Checkbox("Active", &active);
		ImGui::Checkbox("Main Candidate", &mainCandidate);
		ImGui::DragInt("Priority", &priority);
		
		ImGui::DragFloat("FOV Y", &fovY, 0.1f, 1.0f, 179.0f);
		ImGui::DragFloat("Near Z", &nearZ, 0.01f, 0.001f, 100.0f);
		ImGui::DragFloat("Far Z", &farZ, 1.0f, 1.0f, 10000.0f);

		ImGui::TreePop();
	}
}

void DirectionalLightComponent::OnInspectorGui()
{
	if (ImGui::TreeNode("Directional Light")) {
		ImGui::ColorEdit3("Color", &color.x);
		ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);
		ImGui::ColorEdit3("Ambient", &ambient.x);

		bool changed = false;
		if (ImGui::SliderFloat("Elevation (Pitch)", &elevation, 0.0f, 90.0f)) changed = true;
		if (ImGui::SliderFloat("Azimuth (Yaw)", &azimuth, 0.0f, 360.0f)) changed = true;

		if (changed) {
			UpdateTransform();
		}

		ImGui::Separator();
		ImGui::Checkbox("Cast Shadow", &castShadow);
		if (castShadow) {
			ImGui::DragFloat("Ortho Size", &orthoHalfSize, 0.1f, 1.0f, 1000.0f);
			ImGui::DragFloat("Near Z", &nearZ, 0.1f, 0.01f, 1000.0f);
			ImGui::DragFloat("Far Z", &farZ, 1.0f, 1.0f, 5000.0f);
		}

		ImGui::TreePop();
	}
}

void ShadowComponent::OnInspectorGui() {
	if (ImGui::TreeNode("Shadow")) {
		ImGui::Checkbox("Cast Shadow", &castShadow);
		
		static const uint32_t sizes[] = { 512, 1024, 2048, 4096 };
		static const char* sizeNames[] = { "512", "1024", "2048", "4096" };
		
		int currentIdx = 2; // Default 2048
		for(int i=0; i<4; ++i) {
			if(shadowMapSize == sizes[i]) {
				currentIdx = i;
				break;
			}
		}
		
		if(ImGui::Combo("Shadow Map Size", &currentIdx, sizeNames, 4)) {
			shadowMapSize = sizes[currentIdx];
		}

		ImGui::Separator();
		ImGui::Text("Directional Light Settings (CSM)");
		ImGui::DragFloat("Ortho Half Size", &orthoHalfSize, 0.1f, 1.0f, 1000.0f);
		ImGui::DragFloat("Near Z", &nearZ, 0.1f, 0.01f, 1000.0f);
		ImGui::DragFloat("Far Z", &farZ, 1.0f, 1.0f, 5000.0f);

		ImGui::TreePop();
	}
}

void PointLightComponent::OnInspectorGui()
{
	if (ImGui::TreeNode("Point Light")) {
		ImGui::ColorEdit3("Color", &color.x);
		ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);
		ImGui::DragFloat("Range", &range, 0.1f, 0.0f);
		ImGui::TreePop();
	}
}

void SpotLightComponent::OnInspectorGui()
{
	if (ImGui::TreeNode("Spot Light")) {
		ImGui::ColorEdit3("Color", &color.x);
		ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);
		ImGui::DragFloat("Range", &range, 0.1f, 0.0f);
		
		float innerDeg = Math::Func::NUM::ToDegrees(std::acos(innerCos));
		float outerDeg = Math::Func::NUM::ToDegrees(std::acos(outerCos));

		if (ImGui::DragFloat("Inner Angle", &innerDeg, 0.1f, 0.0f, 180.0f)) {
			innerCos = std::cos(Math::Func::NUM::ToRadians(innerDeg));
		}
		if (ImGui::DragFloat("Outer Angle", &outerDeg, 0.1f, 0.0f, 180.0f)) {
			outerCos = std::cos(Math::Func::NUM::ToRadians(outerDeg));
		}

		ImGui::Separator();
		ImGui::Checkbox("Cast Shadow", &castShadow);
		if (castShadow) {
			ImGui::DragFloat("Near Z", &nearZ, 0.1f, 0.01f, 1000.0f);
			ImGui::DragFloat("Far Z", &farZ, 1.0f, 1.0f, 5000.0f);
		}
		ImGui::TreePop();
	}
}

void MaterialComponent::OnInspectorGui()
{
	if (ImGui::TreeNode("Material")) {
		// Surface Type
		int surfaceType = static_cast<int>(surface);
		const char* items[] = { "Opaque", "Transparent" }; // Assuming SurfaceType enum order
		if (ImGui::Combo("Surface", &surfaceType, items, IM_ARRAYSIZE(items))) {
			surface = static_cast<SurfaceType>(surfaceType);
		}

		ImGui::ColorEdit3("Base Color", &baseColor.x);
		ImGui::DragFloat("Alpha", &alpha, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Roughness", &roughness, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Metallic", &metallic, 0.01f, 0.0f, 1.0f);
		
		ImGui::Text("Textures (ToDo: Select from Resource Browser)");
		ImGui::LabelText("Albedo", "%s", albedo.c_str());

		ImGui::TreePop();
	}
}

void RenderComponent::OnInspectorGui()
{
	if (ImGui::TreeNode("Render")) {
		ImGui::Checkbox("Visible", &visible);
		ImGui::Checkbox("Cast Shadow", &castShadow);
		
		ImGui::Text("Mesh: %s", mesh.c_str());

		ImGui::TreePop();
	}
}

}

#include "Framework/Component/Transform/TransformComponent.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/Light/DirectionalLightComponent.h"
#include "Framework/Component/Light/PointLightComponent.h"
#include "Framework/Component/Light/SpotLightComponent.h"
#include "Framework/Component/Material/MaterialComponent.h"
#include "Framework/Component/Render/RenderComponent.h"

#include "imgui.h"

namespace Tsumi::Framework {

void TransformComponent::OnInspectorGui() {
	if (ImGui::TreeNode("Transform")) {
		bool isChanged = false;
		// Scaleは0.01fを最小値に設定（消失・反転防止）
		if (ImGui::DragFloat3("Scale", &srt.scale.x, 0.01f, 0.01f, 10000.0f)) isChanged = true;
		if (ImGui::DragFloat3("Rotation", &srt.rotate.x, 0.1f)) isChanged = true;
		if (ImGui::DragFloat3("Translate", &srt.translate.x, 0.1f)) isChanged = true;

		if (isChanged) {
			MarkDirty();
		}
		ImGui::TreePop();
	}
}

void CameraComponent::OnInspectorGui() {
	if (ImGui::TreeNode("Camera")) {
		ImGui::Checkbox("Active", &active);
		ImGui::Checkbox("Main Candidate", &mainCandidate);
		ImGui::DragInt("Priority", &priority);

		// FOVは1度〜179度の範囲に制限
		ImGui::DragFloat("FOV Y", &fovY, 0.1f, 1.0f, 179.0f);
		// Nearは0より大きく、Farより小さい必要があるため最小値を設定
		ImGui::DragFloat("Near Z", &nearZ, 0.01f, 0.01f, 10.0f);
		// FarはNearより大きい必要がある
		ImGui::DragFloat("Far Z", &farZ, 1.0f, 0.1f, 10000.0f);

		ImGui::TreePop();
	}
}

void DirectionalLightComponent::OnInspectorGui() {
	if (ImGui::TreeNode("Directional Light")) {
		ImGui::ColorEdit3("Color", &color.x);
		ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);
		ImGui::ColorEdit3("Ambient", &ambient.x);

		if (ImGui::TreeNode("Shadow")) {
			ImGui::Checkbox("Cast Shadow", &shadow.castShadow);
			ImGui::DragFloat("Ortho Size", &shadow.orthoHalfSize, 0.1f, 0.1f, 1000.0f);
			ImGui::DragFloat("Near Z", &shadow.nearZ, 0.1f, 0.01f, 1000.0f);
			ImGui::DragFloat("Far Z", &shadow.farZ, 0.1f, 0.1f, 5000.0f);
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}
}

void PointLightComponent::OnInspectorGui() {
	if (ImGui::TreeNode("Point Light")) {
		ImGui::ColorEdit3("Color", &color.x);
		ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);
		// 範囲がマイナスになると計算が破綻するため最小値 0.0f
		ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 1000.0f);
		ImGui::TreePop();
	}
}

void SpotLightComponent::OnInspectorGui() {
	if (ImGui::TreeNode("Spot Light")) {
		ImGui::ColorEdit3("Color", &color.x);
		ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 100.0f);
		ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 1000.0f);

		float innerDeg = Math::Func::NUM::ToDegrees(std::acos(innerCos));
		float outerDeg = Math::Func::NUM::ToDegrees(std::acos(outerCos));

		// 角度は 0〜180度（またはそれ以下）に制限
		if (ImGui::DragFloat("Inner Angle", &innerDeg, 0.1f, 0.0f, 180.0f)) {
			innerCos = std::cos(Math::Func::NUM::ToRadians(innerDeg));
		}
		if (ImGui::DragFloat("Outer Angle", &outerDeg, 0.1f, 0.0f, 180.0f)) {
			outerCos = std::cos(Math::Func::NUM::ToRadians(outerDeg));
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

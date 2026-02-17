#include "EngineUI.h"
#include "imgui.h"
#include "../../Game/GameContext.h"
#include "Framework/World/World.h"
#include "Framework/Actor/IActor.h"
#include "Resource/ResourceSystem.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"

using namespace Tsumi::Editor;

void EngineUI::Init(Utils::FixFPS* fixFPS)
{
	fixFPS_ = fixFPS;
}

void EngineUI::Draw()
{
	ImGui::Begin("Tsumi Engine UI");

	if (ImGui::BeginTabBar("EngineUITabBar")) {
		
		if (ImGui::BeginTabItem("Performance")) {
			DrawPerformance();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Scene")) {
			// Left pane for Scene Graph
			ImGui::BeginChild("SceneGraph", { ImGui::GetContentRegionAvail().x * 0.4f, 0 }, true);
			DrawScene();
			ImGui::EndChild();

			ImGui::SameLine();

			// Right pane for Inspector
			ImGui::BeginChild("Inspector", { 0, 0 }, true);
			DrawInspector();
			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Resources")) {
			DrawResources();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Debug")) {
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

void EngineUI::DrawPerformance()
{
	if (fixFPS_) {
		float fps = fixFPS_->GetFPS();
		ImGui::Text("FPS: %.2f", fps);
		ImGui::Text("Frame Time: %.2f ms", 1000.0f / fps);
	}
	else {
		ImGui::Text("FPS: N/A");
	}
}

void EngineUI::DrawScene()
{
	if (!gameContext_) {
		ImGui::Text("No Game Context");
		return;
	}

	auto* world = gameContext_->GetWorld();
	if (!world) {
		ImGui::Text("No Active World");
		return;
	}

	const auto& actors = world->GetActors();
	for (const auto& actor : actors) {
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (selectedActor_ == actor.get()) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}
		
		// Leaf node for now (no hierarchy visualization yet, confusing with parent/child transform)
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		ImGui::TreeNodeEx((void*)actor.get(), flags, "%s", actor->GetName().c_str());
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			selectedActor_ = actor.get();
		}
	}
}

void EngineUI::DrawInspector()
{
	if (!selectedActor_) {
		ImGui::Text("No Actor Selected");
		return;
	}

	// Name editing
	char nameBuf[256];
	strcpy_s(nameBuf, selectedActor_->GetName().c_str());
	if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
		selectedActor_->SetName(nameBuf);
	}

	ImGui::Separator();

	// Components
	selectedActor_->ForEachComponent([](Tsumi::Framework::IComponent* comp) {
		comp->OnInspectorGui();
		ImGui::Separator();
	});
}

void EngineUI::DrawResources()
{
	auto* resSys = Tsumi::Resource::ResourceSystem::GetInstance();
	
	if (ImGui::TreeNode("Meshes")) {
		auto names = resSys->GetMeshManager()->GetMeshNames();
		for (const auto& name : names) {
			ImGui::Text("%s", name.c_str());
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Textures")) {
		auto names = resSys->GetTextureManager()->GetTextureNames();
		for (const auto& name : names) {
			ImGui::Text("%s", name.c_str());
		}
		ImGui::TreePop();
	}
}

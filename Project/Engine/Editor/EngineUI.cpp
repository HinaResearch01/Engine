#include "EngineUI.h"
#include "imgui.h"
#include "../../Game/GameContext.h"
#include "Framework/World/World.h"
#include "Framework/Actor/IActor.h"
#include "Resource/ResourceSystem.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"
#include "Framework/System/Render/RenderSystem.h"

using namespace Tsumi::Editor;

void EngineUI::Init(Utils::FixFPS* fixFPS)
{
	fixFPS_ = fixFPS;
}

void EngineUI::Draw()
{
	// DockSpace Setup
	// Create a window that covers the entire viewport
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("Tsumi Engine DockSpace", nullptr, window_flags);
	ImGui::PopStyleVar(3);

	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("View"))
		{
			// Add menu items to toggle windows if needed
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	ImGui::End();

	// Individual Windows
	if (ImGui::Begin("Performance")) {
		DrawPerformance();
	}
	ImGui::End();

	if (ImGui::Begin("Scene")) {
		// Layout Scene Graph and Inspector
		// For now, let's keep them together or separate? User might want them separate.
		// Let's separate them.
		DrawScene();
	}
	ImGui::End();

	if (ImGui::Begin("Inspector")) {
		DrawInspector();
	}
	ImGui::End();

	if (ImGui::Begin("Resources")) {
		DrawResources();
	}
	ImGui::End();

	if (ImGui::Begin("Debug")) {
		DrawShadowDebug();
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

void EngineUI::DrawShadowDebug()
{
	if (!gameContext_) return;
	auto* world = gameContext_->GetWorld();
	if (!world) return;

	auto* renderSys = world->GetSystem<Tsumi::Framework::RenderSystem>();
	if (!renderSys) return;

	auto* shadowMap = renderSys->GetShadowMap();
	if (!shadowMap) {
		ImGui::Text("No Shadow Map active");
		return;
	}

	ImGui::Text("Shadow Map Debug View");
	ImGui::Text("Size: %d, Cascades: %d", shadowMap->GetSize(), shadowMap->GetCascadeCount());

	// 2カラムレイアウトで表示
	float windowWidth = ImGui::GetContentRegionAvail().x;
	float spacing = 10.0f;
	float imgSize = (windowWidth - spacing) * 0.5f; 
	if (imgSize < 100.0f) imgSize = 100.0f;

	for (uint32_t i = 0; i < shadowMap->GetCascadeCount(); ++i) {
		ImGui::PushID(i);
		ImGui::BeginGroup(); // グループ化してテキストと画像をセットにする
		
		ImGui::Text("Cascade %d", i);
		
		auto& srv = shadowMap->GetDebugSRV(i);
		if (srv.valid()) {
			// 画像の枠線をつける（視認性向上）
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImGui::Image((ImTextureID)srv.gpu.ptr, { imgSize, imgSize }, { 0,0 }, { 1,1 }, { 1,1,1,1 }, { 1,1,1,1 });
			
			// 枠線描画
			ImGui::GetWindowDrawList()->AddRect(cursor, { cursor.x + imgSize, cursor.y + imgSize }, IM_COL32(255, 255, 255, 100));
		}
		else {
			ImGui::Dummy({ imgSize, imgSize });
			ImGui::Text("Invalid SRV");
		}
		
		ImGui::EndGroup();
		ImGui::PopID();
		
		// 偶数（0, 2...）の次は改行しない = SameLine
		if (i % 2 == 0 && i < shadowMap->GetCascadeCount() - 1) {
			ImGui::SameLine(0.0f, spacing);
		}
	}
	ImGui::NewLine();
}
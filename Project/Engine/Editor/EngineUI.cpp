#include "EngineUI.h"
#include "imgui.h"

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
			DrawScene();
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
	ImGui::Text("Scene Graph (ToDo)");
}

void EngineUI::DrawResources()
{
	ImGui::Text("Texture Count: (ToDo)");
	ImGui::Text("Model Count: (ToDo)");
}

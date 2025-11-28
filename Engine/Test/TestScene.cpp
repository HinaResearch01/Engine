#include "TestScene.h"
#include "imgui.h"

void TestScene::Init()
{
}

void TestScene::Update()
{

#ifdef _DEBUG
	ImGui::Begin("TestScene : テストシーン");
	if (ImGui::TreeNode("Player : プレイヤー")) {

		ImGui::TreePop();
	}
	ImGui::End();
#endif // _DEBUG
}

void TestScene::RenderModel()
{
}

void TestScene::Finalize()
{
}

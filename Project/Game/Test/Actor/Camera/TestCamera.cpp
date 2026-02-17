#include "TestCamera.h"
#include "TestCameraComp.h"

void TestCamera::Init()
{
	IActor::name_ = "TestCamera";
	IActor::AddComp<TestCameraComp>();
	auto* transComp = IActor::GetComponent<Tsumi::Framework::TransformComponent>();
	transComp->srt.rotate = { 30.0f, 0.0f, 0.0f };
	transComp->srt.translate = { 0.0f, 45.0f, -60.0f };
}

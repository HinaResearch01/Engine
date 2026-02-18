#include "TestCamera.h"
#include "TestCameraComp.h"

void TestCamera::Init()
{
	IActor::name_ = "TestCamera";
	IActor::AddComp<TestCameraComp>();
	auto* transComp = IActor::GetComponent<Tsumi::Framework::TransformComponent>();
	transComp->srt.rotate = { -20.0f, 40.0f, 0.0f };
	transComp->srt.translate = { 20.0f, 14.0f, -12.0f };
}

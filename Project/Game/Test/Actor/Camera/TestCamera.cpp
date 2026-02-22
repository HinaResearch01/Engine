#include "TestCamera.h"
#include "TestCameraComp.h"
#include "TestCameraMoveComp.h"

void TestCamera::Init()
{
	IActor::name_ = "TestCamera";
	IActor::AddComp<TestCameraComp>();
	IActor::AddComp<TestCameraMoveComp>();
	auto* transComp = IActor::GetComponent<Tsumi::Framework::TransformComponent>();
	transComp->srt.rotate = { -26.0f, 0.0f, 0.0f };
	transComp->srt.translate = { 0.0f, 35.0f, -60.0f };
}

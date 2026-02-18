#include "TestFloor.h"
#include "TestFloorMaterialComp.h"
#include "TestFloorRenderComp.h"

void TestFloor::Init()
{
	IActor::name_ = "TestFloor";
	IActor::AddComp<TestFloorMaterialComp>();
	auto* rendComp = IActor::AddComp<TestFloorRenderComp>();
	rendComp->mesh = "floor";
	auto* transComp = IActor::GetComponent<Tsumi::Framework::TransformComponent>();
	transComp->srt.scale = { 100.0f, 1.0f, 100.0f };
}

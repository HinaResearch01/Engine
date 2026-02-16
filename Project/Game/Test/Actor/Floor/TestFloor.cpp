#include "TestFloor.h"
#include "TestFloorMaterialComp.h"
#include "TestFloorRenderComp.h"

void TestFloor::Init()
{
	IActor::name_ = "TestFloor";
	IActor::AddComp<TestFloorMaterialComp>();
	auto* rendComp = IActor::AddComp<TestFloorRenderComp>();
	rendComp->mesh = "floor";
}

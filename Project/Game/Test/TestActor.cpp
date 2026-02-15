#include "TestActor.h"
#include "TestMaterialComponent.h"
#include "TestRenderComponent.h"

void TestActor::Init()
{
	IActor::name_ = "TestActor";

	IActor::AddComp<TestMaterialComponent>();

	auto* rendComp = IActor::AddComp<TestRenderComponent>();
	rendComp->mesh = "TestCube";
}	
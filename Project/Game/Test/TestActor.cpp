#include "TestActor.h"
#include "TestMaterialComponent.h"
#include "TestRenderComponent.h"

void TestActor::Init()
{
	IActor::name_ = "TestActor";
	IActor::AddComp<TestMaterialComponent>();
}
#include "TestActor.h"
#include "TestMaterialComponent.h"
#include "TestRenderComponent.h"

void TestActor::Init()
{
	IActor::AddComp<TestMaterialComponent>();
}
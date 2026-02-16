#include "TestCylinder.h"
#include "TestCylinderMaterialComp.h"
#include "TestCylinderRenderComp.h"

void TestCylinder::Init()
{
	IActor::name_ = "TestCylinder";
	IActor::AddComp<TestCylinderMaterialComp>();
	auto* rendComp = IActor::AddComp<TestCylinderRenderComp>();
	rendComp->mesh = "cylinder";
}
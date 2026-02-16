#include "TestAxis.h"
#include "TestAxisMaterialComp.h"
#include "TestAxisRenderComp.h"

void TestAxis::Init()
{
	IActor::name_ = "TestAxis";
	IActor::AddComp<TestAxisMaterialComp>();
	auto* rendComp = IActor::AddComp<TestAxisRenderComp>();
	rendComp->mesh = "axis";
}	
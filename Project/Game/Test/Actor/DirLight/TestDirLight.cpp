#include "TestDirLight.h"
#include "TestDiretionalLightComp.h"
#include "TestShadowComp.h"

void TestDirLight::Init()
{
	IActor::name_ = "TestDirLight";
	IActor::AddComp<TestDirectionalLightComp>();
	IActor::AddComp<TestShadowComp>();
}

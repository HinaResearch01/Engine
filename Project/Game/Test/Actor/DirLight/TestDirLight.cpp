#include "TestDirLight.h"
#include "TestDiretionalLightComp.h"

void TestDirLight::Init()
{
	IActor::name_ = "TestDirLight";
	IActor::AddComp<TestDirectionalLightComp>();
}

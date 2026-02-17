#include "TestSpotLight.h"
#include "TestSpotLightComp.h"

void TestSpotLight::Init()
{
	IActor::name_ = "TestSpotLight";
	IActor::AddComp<TestSpotLightComp>();
}

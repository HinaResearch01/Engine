#pragma once

#include "Framework/Actor/IActor.h"

class TestSpotLight : public Tsumi::Framework::IActor {

public:
	TestSpotLight() = default;
	~TestSpotLight() = default;
	void Init() override;
};


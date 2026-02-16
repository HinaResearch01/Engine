#pragma once

#include "Framework/Actor/IActor.h"
#include "TestCameraComponent.h"

class TestCamera : public Tsumi::Framework::IActor {

public:
	TestCamera() = default;
	~TestCamera() = default;
	void Init() override;
};

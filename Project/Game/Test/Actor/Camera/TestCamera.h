#pragma once

#include "Framework/Actor/IActor.h"

class TestCamera : public Tsumi::Framework::IActor {

public:
	TestCamera() = default;
	~TestCamera() = default;
	void Init() override;
};

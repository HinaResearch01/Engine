#pragma once

#include "Framework/Actor/IActor.h"

class TestCylinder : public Tsumi::Framework::IActor {

public:
	TestCylinder() = default;
	~TestCylinder() = default;
	void Init() override;
};

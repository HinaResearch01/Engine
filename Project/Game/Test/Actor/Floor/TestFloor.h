#pragma once

#include "Framework/Actor/IActor.h"

class TestFloor : public Tsumi::Framework::IActor {

public:
	TestFloor() = default;
	~TestFloor() = default;
	void Init() override;
};


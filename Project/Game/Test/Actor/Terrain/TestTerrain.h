#pragma once

#include "Framework/Actor/IActor.h"

class TestTerrain : public Tsumi::Framework::IActor {

public:
	TestTerrain() = default;
	~TestTerrain() = default;
	void Init() override;
};


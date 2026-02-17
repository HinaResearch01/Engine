#pragma once

#include "Framework/Actor/IActor.h"

class TestDirLight : public Tsumi::Framework::IActor {

public:
	TestDirLight() = default;
	~TestDirLight() = default;
	void Init() override;
};


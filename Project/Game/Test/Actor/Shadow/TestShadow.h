#pragma once

#include "Framework/Actor/IActor.h"

class TestShadow : public Tsumi::Framework::IActor {

public:
	TestShadow() = default;
	~TestShadow() = default;
	void Init() override;
};


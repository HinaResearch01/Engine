#pragma once

#include "Framework/Actor/IActor.h"

class TestAxis : public Tsumi::Framework::IActor {

public:
    TestAxis() = default;
    ~TestAxis() = default;
	void Init() override;
};
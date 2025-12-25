#pragma once

#include "Framework/Actor/IActor.h"

class TestActor : public Tsumi::Framework::IActor {

public:
    TestActor() = default;
    ~TestActor() = default;

	void Init() override;
private:
};
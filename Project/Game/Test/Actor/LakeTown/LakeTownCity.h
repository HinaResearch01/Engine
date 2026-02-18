#pragma once

#include "Framework/Actor/IActor.h"

class LakeTownCity : public Tsumi::Framework::IActor {

public:
	LakeTownCity() = default;
	~LakeTownCity() = default;
	void Init() override;
};


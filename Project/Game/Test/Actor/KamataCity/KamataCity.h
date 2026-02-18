#pragma once

#include "Framework/Actor/IActor.h"

class KamataCity : public Tsumi::Framework::IActor {

public:
	KamataCity() = default;
	~KamataCity() = default;
	void Init() override;
};


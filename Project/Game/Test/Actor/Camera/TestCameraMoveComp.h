#pragma once

#include "Framework/Component/IComponent.h"
#include "Framework/Update/IUpdatable.h"
#include "Math/TMath.h"

class TestCameraMoveComp : public Tsumi::Framework::IComponent, public Tsumi::Framework::IUpdatable {

public:
	TestCameraMoveComp() = default;
	~TestCameraMoveComp() = default;

	void Update(float dt) override;
	Tsumi::Framework::UpdatePhase Phase() const override { return Tsumi::Framework::UpdatePhase::Logic; }

private:
	float moveSpeed_ = 15.0f;
	float rotateSpeed_ = 30.0f;
};

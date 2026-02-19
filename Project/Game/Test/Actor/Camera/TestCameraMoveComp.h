#pragma once
#include "Framework/Component/IComponent.h"
#include "Math/TMath.h"

class TestCameraMoveComp : public Tsumi::Framework::IComponent, Tsumi::Framework::IUpdatable {

public:
	TestCameraMoveComp() = default;
	~TestCameraMoveComp() = default;
	void Init() override;
	void Update(float deltaTime) override;

private:
	float moveSpeed_ = 10.0f;
	float rotateSpeed_ = 2.0f;
};

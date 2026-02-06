#pragma once

#include "Framework/World/World.h"
#include "TestActor.h"

/* テスト用のsceneクラス */
class TestWorld : public Tsumi::Framework::World {

public:
	TestWorld() = default;
	~TestWorld() = default;
	void Init() override;
	void Update(float deltaTime) override;
};


#pragma once

#include "Framework/World/World.h"

/* テスト用のsceneクラス */
class TestScene : public Tsumi::Framework::World {

public:
	TestScene() = default;
	~TestScene() = default;

	void Init() override;
	void Update(float deltaTime) override;
	void Finalize() override;

private:

};


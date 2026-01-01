#pragma once

#include "Framework/Scene/IScene.h"

/* テスト用のsceneクラス */
class TestScene : public Tsumi::Framework::IScene {

public:
	TestScene() = default;
	~TestScene() = default;

	void Init() override;
	void Update(float deltaTime) override;
	void Finalize() override;

private:

};


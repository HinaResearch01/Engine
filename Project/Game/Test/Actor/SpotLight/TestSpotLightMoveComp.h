#pragma once

#include "Framework/Component/IComponent.h"
#include "Framework/Update/IUpdatable.h"
#include "Framework/Component/Transform/TransformComponent.h"
#include "Math/TMath.h"
#include <random>

class TestSpotLightMoveComp : public Tsumi::Framework::IComponent, public Tsumi::Framework::IUpdatable {

public:
	TestSpotLightMoveComp() = default;
	~TestSpotLightMoveComp() = default;

	void Init() override {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
		std::uniform_real_distribution<float> speedDist(5.0f, 15.0f);

		velocity_ = tme::math::Vec3f(dist(gen), 0.0f, dist(gen)).Normalized() * speedDist(gen);
	}

	void Update(float dt) override {
		auto* owner = GetOwner();
		auto* tr = owner->GetComponent<Tsumi::Framework::TransformComponent>();
		if (!tr) return;

		tr->srt.translate += velocity_ * dt;

		// 境界チェック (簡易的な跳ね返り)
		float limit = 40.0f;
		if (std::abs(tr->srt.translate.x) > limit) {
			velocity_.x *= -1.0f;
			tr->srt.translate.x = (tr->srt.translate.x > 0) ? limit : -limit;
		}
		if (std::abs(tr->srt.translate.z) > limit) {
			velocity_.z *= -1.0f;
			tr->srt.translate.z = (tr->srt.translate.z > 0) ? limit : -limit;
		}

		tr->MarkDirty();
	}

	Tsumi::Framework::UpdatePhase Phase() const override { return Tsumi::Framework::UpdatePhase::Logic; }

private:
	tme::math::Vec3f velocity_;
};

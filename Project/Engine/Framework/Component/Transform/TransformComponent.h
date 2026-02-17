#pragma once

#include "Math/TMath.h"
#include "../IComponent.h"

namespace Tsumi::Framework {

struct SRT {
	Math::Vec3f scale{};
	Math::Vec3f rotate{};
	Math::Vec3f translate{};
	SRT() :
		scale{ 1.0f, 1.0f, 1.0f },
		rotate{ 0.0f, 0.0f, 0.0f },
		translate{ 0.0f, 0.0f, 0.0f }
	{}
};

/* Actorの「位置・回転・スケール」を管理 データコンポーネント */
class TransformComponent : public IComponent {

public:
	void OnInspectorGui() override;

	Math::Vec3f GetWorldPos() const {
		return { world.m[3][0], world.m[3][1], world.m[3][2] };
	}

	void MarkDirty() {
		selfDirty = true;
		worldDirty = true;
	}

public:
	// 入力
	SRT srt{};

	// dirty flags
	bool selfDirty = true;
	bool worldDirty = true;

	// 派生
	Math::Mat4x4 world{};
	Math::Mat4x4 worldInvTranspose{};
	Math::Vec3f right{ 1,0,0 };
	Math::Vec3f up{ 0,1,0 };
	Math::Vec3f forward{ 0,0,1 };

	// 階層
	TransformComponent* parent = nullptr;
};

}
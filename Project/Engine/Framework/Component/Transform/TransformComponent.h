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
	// 自身のSRTが変更されたか
	bool IsSelfDirty() const {
		return std::memcmp(&srt, &prevLocal, sizeof(SRT)) != 0;
	}

	// 現在のSRTを前回のSRTに同期
	void SyncPrev() {
		prevLocal = srt;
	}

	// ワールド位置の取得
	Math::Vec3f GetWorldPos() const {
		return { world.m[3][0], world.m[3][1], world.m[3][2] };
	}

public:
	// ===== 入力 =====
	SRT srt{};
	SRT prevLocal{};

	// ===== 派生（TransformSystem が更新）=====
	Math::Mat4x4 world{};

	Math::Vec3f right{ 1.0f, 0.0f, 0.0f };
	Math::Vec3f up{ 0.0f, 1.0f, 0.0f };
	Math::Vec3f forward{ 0.0f, 0.0f, 1.0f };

	// ===== 階層 =====
	std::weak_ptr<TransformComponent> parent;
	bool parentDirty = true;
};

}
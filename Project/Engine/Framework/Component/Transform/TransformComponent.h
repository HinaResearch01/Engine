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

/* Actorの「位置・回転・スケール」を管理 */
class TransformComponent : public IComponent {

public:
	SRT local{};                 // ローカルSRT
	Math::Mat4x4 world{};        // ワールド行列

	Math::Vec3f forward{ 0,0,1 };
	Math::Vec3f up{ 0,1,0 };

	std::weak_ptr<TransformComponent> parent;

	// dirty 判定用
	SRT prevLocal{};

	bool IsDirty() const {
		return memcmp(&local, &prevLocal, sizeof(SRT)) != 0;
	}

	void MarkClean() {
		prevLocal = local;
	}
};

}
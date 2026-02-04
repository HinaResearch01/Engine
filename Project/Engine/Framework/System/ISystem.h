#pragma once

#include "Framework/Update/IUpdatable.h"

namespace Tsumi::Framework {

// 前方宣言
class World;

/* 各システムの基底クラス */
class ISystem : public IUpdatable {
public:
	virtual ~ISystem() = default;
};
}
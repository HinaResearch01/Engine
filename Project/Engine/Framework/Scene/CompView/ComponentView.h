#pragma once

#include <memory>
#include <vector>
#include "Framework/Actor/IActor.h"
#include "Framework/Component/IComponent.h"

namespace Tsumi::Framework {

/* 指定コンポーネントを持つアクターの一覧を管理するビュークラス */
template<typename T>
class ComponentView {

public:
	const std::vector<IActor*>& Get() const { return actors_; }

	void Add(IActor* actor) {
		actors_.push_back(actor);
	}

	void Remove(IActor* actor) {
		std::erase(actors_, actor);
	}

private:
	std::vector<IActor*> actors_;
};

}
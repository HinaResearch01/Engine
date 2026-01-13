#pragma once

#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cassert>
#include "IComponentViewBase.h"
#include "Framework/Actor/IActor.h"

namespace Tsumi::Framework {

/* 指定コンポーネント T を「持っている Actor」だけを索引する View */
template<class T>
class ComponentView : public IComponentViewBase {
public:
	/// <summary>
	/// System 用：列挙専用アクセサ
	/// </summary>
	const std::vector<IActor*>& GetActors() const {
		return actors_;
	}

	/// <summary>
	/// 全消去
	/// </summary>
	void Clear() override {
		actors_.clear();
		set_.clear();
	}

	/// <summary>
	/// view に入れる
	/// Component の追加 / 削除時に呼ばれる
	/// </summary>
	void Refresh(IActor* actor) override {
		if (!actor) return;

		const bool hasComp = actor->HasComp<T>();
		const bool inView = (set_.find(actor) != set_.end());

		if (hasComp && !inView) {
			Add(actor);
		}
		else if (!hasComp && inView) {
			Remove(actor);
		}
		// else : 状態変化なし → 何もしない
	}

	/// <summary>
	/// view から外す
	/// Actor 破棄時などに使用
	/// </summary>
	void Remove(IActor* actor) override {
		if (!actor) return;

		if (set_.erase(actor) == 0)
			return; // そもそも入っていない

		std::erase(actors_, actor);
	}

private:
	void Add(IActor* actor) {
		assert(actor);
		assert(set_.find(actor) == set_.end()); // 二重登録防止（Debug用）

		set_.insert(actor);
		actors_.push_back(actor);
	}

private:
	std::vector<IActor*> actors_;          // 走査用（順序保持）
	std::unordered_set<IActor*> set_;      // 存在判定・重複防止
};

}

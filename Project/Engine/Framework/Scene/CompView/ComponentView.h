#pragma once

#include <vector>
#include <unordered_set>
#include <algorithm>
#include "Framework/Actor/IActor.h"
#include "Framework/Component/IComponent.h"

namespace Tsumi::Framework {

/* 指定コンポーネントを持つアクターの一覧を管理するビュークラス */
template<class T>
class ComponentView {
public:
	// 取得：System はこれだけを見る
	const std::vector<IActor*>& Get() const { return actors_; }

	// 全消去（Scene::Finalizeなど）
	void Clear() {
		actors_.clear();
		set_.clear();
	}

	/*
		Refresh(actor)
		- actor が T を持つなら view に入れる
		- actor が T を持たないなら view から外す
		- 既に入っている/入っていない場合は何もしない
	*/
	void Refresh(IActor* actor) {
		if (!actor) return;

		const bool has = actor->HasComp<T>();
		const bool in = (set_.find(actor) != set_.end());

		if (has && !in) {
			Add(actor);
		}
		else if (!has && in) {
			Remove(actor);
		}
	}

	/*
		Remove(actor)
		- 強制的に view から外す（actor削除時など）
		- actor が入っていなければ何もしない
	*/
	void Remove(IActor* actor) {
		if (!actor) return;
		if (set_.erase(actor) == 0) return;

		// actors_ からも削除（線形だが、viewはカテゴリ毎の小さい集合になる想定）
		std::erase(actors_, actor);
	}

private:
	void Add(IActor* actor) {
		set_.insert(actor);
		actors_.push_back(actor);
	}

private:
	std::vector<IActor*> actors_;          // 非所有：順序保持、走査用
	std::unordered_set<IActor*> set_;      // 非所有：重複防止・存在判定用
};

}
#pragma once

#include <vector>
#include <algorithm>
#include "UpdatePhase.h"
#include "IUpdatable.h"

namespace Tsumi::Framework {

/* 更新管理者 */
class UpdateManager {

public:
	/// <summary>
	/// 登録処理
	/// </summary>
	void Register(IUpdatable* u) {
		if (!u) return;
		auto& list = lists_[static_cast<size_t>(u->Phase())];
		list.push_back(u);
	}

	/// <summary>
	/// 解除処理
	/// </summary>
	void UnRegister(IUpdatable* u) {
		if (!u) return;
		auto& list = lists_[static_cast<size_t>(u->Phase())];
		std::erase(list, u);
	}

	/// <summary>
	/// 更新処理
	/// </summary>
	void Execute(float deltaTime) {
		for (size_t phase = 0; phase < static_cast<size_t>(UpdatePhase::Count); ++phase) {
			auto& list = lists_[phase];
			// 優先度順にソート（大きいほど先）
			std::sort(list.begin(), list.end(), [](IUpdatable* a, IUpdatable* b) {
				return a->Priority() > b->Priority();
			});
			// 更新処理実行
			for (auto* u : list) {
				if (u->Enabled()) {
					u->Update(deltaTime);
				}
			}
		}
	}

	/// <summary>
	/// デバッグ・可視化用
	/// </summary>
	const std::vector<IUpdatable*>& GetList(UpdatePhase phase) const {
		return lists_[static_cast<size_t>(phase)];
	}

private:
	std::vector<IUpdatable*> lists_[static_cast<size_t>(UpdatePhase::Count)];
};

}
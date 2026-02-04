#pragma once

#include <vector>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include "UpdatePhase.h"
#include "IUpdatable.h"

namespace Tsumi::Framework {

/* 更新管理者 */
class UpdateManager {

public:
	void Register(IUpdatable* u) {
		if (!u) return;

		if (executing_) {
			pendingReg_.push_back(u);
			return;
		}
		RegisterImmediate(u);
	}

	void UnRegister(IUpdatable* u) {
		if (!u) return;

		if (executing_) {
			pendingUnreg_.push_back(u);
			return;
		}
		UnregisterImmediate(u);
	}

	void Execute(float dt) {
		executing_ = true;

		// phase順に実行
		for (size_t p = 0; p < static_cast<size_t>(UpdatePhase::Count); ++p) {
			auto& list = lists_[p];

			if (dirty_[p]) {
				std::sort(list.begin(), list.end(),
						  [](IUpdatable* a, IUpdatable* b) { return a->Priority() > b->Priority(); });
				dirty_[p] = false;
			}

			// 走査中のRegister/UnRegisterは遅延キューへ
			for (IUpdatable* u : list) {
				if (!u) continue;
				if (u->Enabled()) {
					u->Update(dt);
				}
			}
		}

		executing_ = false;
		FlushPending();
	}

	const std::vector<IUpdatable*>& GetList(UpdatePhase phase) const {
		return lists_[static_cast<size_t>(phase)];
	}

private:
	using PhaseIndex = uint8_t;

	void RegisterImmediate(IUpdatable* u) {
		// 二重登録防止
		auto it = registeredPhase_.find(u);
		const UpdatePhase desired = u->Phase();

		if (it != registeredPhase_.end()) {
			// Phaseが変わったケース：移動対応
			if (it->second != desired) {
				MovePhase(u, it->second, desired);
			}
			return; // 既に登録済み
		}

		auto& list = lists_[static_cast<size_t>(desired)];
		list.push_back(u);

		registeredPhase_[u] = desired;
		dirty_[static_cast<size_t>(desired)] = true;
	}

	void UnregisterImmediate(IUpdatable* u) {
		auto it = registeredPhase_.find(u);
		if (it == registeredPhase_.end()) return;

		const UpdatePhase phase = it->second;
		auto& list = lists_[static_cast<size_t>(phase)];
		std::erase(list, u);

		registeredPhase_.erase(it);
		// sortは不要
	}

	void MovePhase(IUpdatable* u, UpdatePhase from, UpdatePhase to) {
		auto& src = lists_[static_cast<size_t>(from)];
		std::erase(src, u);

		auto& dst = lists_[static_cast<size_t>(to)];
		dst.push_back(u);

		registeredPhase_[u] = to;
		dirty_[static_cast<size_t>(to)] = true;
	}

	void FlushPending() {
		// Unregister → Register の順で安全
		for (auto* u : pendingUnreg_) {
			UnregisterImmediate(u);
		}
		pendingUnreg_.clear();

		for (auto* u : pendingReg_) {
			RegisterImmediate(u);
		}
		pendingReg_.clear();
	}

private:
	std::array<std::vector<IUpdatable*>, static_cast<size_t>(UpdatePhase::Count)> lists_{};
	std::array<bool, static_cast<size_t>(UpdatePhase::Count)> dirty_{};

	bool executing_ = false;
	std::vector<IUpdatable*> pendingReg_;
	std::vector<IUpdatable*> pendingUnreg_;

	// 「登録時のPhase」を固定・記録（問題②）
	std::unordered_map<IUpdatable*, UpdatePhase> registeredPhase_;
};

}
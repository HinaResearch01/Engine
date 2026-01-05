#pragma once

#include "UpdatePhase.h"

namespace Tsumi::Framework {

/* 更新可能インターフェース */
class IUpdatable {

public:
		/// <summary>
		/// 仮想デストラクタ
		/// </summary>
		virtual ~IUpdatable() = default;

		/// <summary>
		/// 
		/// </summary>
		virtual UpdatePhase Phase() const = 0;

		/// <summary>
		/// 
		/// </summary>
		virtual int Priority() const { return 0; } // 同Phase内の順序（大きいほど先）

		/// <summary>
		/// 
		/// </summary>
		virtual bool Enabled() const { return true; } // 無効化（Pauseなど）

		/// <summary>
		/// 更新処理
		/// </summary>
		virtual void Update(float dt) = 0;
};
	
}
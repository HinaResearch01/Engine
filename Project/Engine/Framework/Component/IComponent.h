#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>

namespace Tsumi::Framework {

// 前方宣言
class IActor;

/* Actorが持つComppnentの基底クラス */
class IComponent {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	IComponent() = default;

	/// <summary>
	/// 仮想デストラクタ
	/// </summary>
	virtual ~IComponent() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	virtual void Init() {};

	/// <summary>
	/// Inspector描画
	/// </summary>
	virtual void OnInspectorGui() {};

#pragma region Accessor
	// 親
	IActor* GetOwner() const { return owner_; }
	void SetOwner(IActor* ptr) { owner_ = ptr; } 
#pragma endregion

private:
	// 親
	IActor* owner_ = nullptr;
};

}
#pragma once

#include "../IComponent.h"

namespace Tsumi::Framework {

/* Actorの「描画」を管理 */
class RenderComponent : public IComponent {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	RenderComponent() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~RenderComponent() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init() override;

	/// <summary>
	/// 描画処理
	/// </summary>
	void Update() override;

#pragma region Accessor
	
#pragma endregion 


private:

	/// <summary>
	/// ImGuiの描画
	/// </summary>
	void DrawImGui(std::string label = "");

private:


};

}
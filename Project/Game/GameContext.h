#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <functional>
#include "Framework/World/World.h"
#include "DX12/DX12Manager.h"

namespace Tsumi::Framework {

class GameContext {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	GameContext();

	/// <summary>
	/// デストラクタ
	/// </summary>
	~GameContext() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 各描画処理
	/// </summary>
	void Render(std::function<void()> uiRenderCallBack);

	/// <summary>
	/// 解放処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// シーンの登録
	/// </summary>
	template<typename T, typename... Args>
	void RegisterScene(const std::string&, Args&&... args);

	/// <summary>
	/// シーンの変更
	/// </summary>
	void ChangeScene(const std::string& name);

#pragma region Accessor
	bool GetPedingInit() const { return pendingInit_; }
#pragma endregion

private:
	std::unordered_map<std::string, std::unique_ptr<Framework::World>> world_;
	Framework::World* currentScene_ = nullptr;
	bool pendingInit_ = false;
};


inline void GameContext::Init() { 
	if (!pendingInit_ || !currentScene_) return;
	currentScene_->Init(), pendingInit_ = false;
}
inline void GameContext::Update() {	if(currentScene_) currentScene_->Update(0); }
inline void GameContext::Render(std::function<void()> uiRenderCallBack)
{
	if (!currentScene_) return;

	// DX12Managerの取得
	auto* dx12 = DX12::DX12Manager::GetInstance();

	// 現在のシーンからRenderSystemを取得
	auto* renderSys = currentScene_->GetSystem<RenderSystem>();

	// コマンドコンテキスト取得
	auto* cmdContext = dx12->GetCommandContext();

	// -------------------------------------------------
	// 1. Primary Effect Pass (3D描画)
	// -------------------------------------------------
	dx12->BeginGBufferPass();

	if (renderSys) {
		renderSys->RenderBackSprite(*cmdContext); // 背景
		renderSys->RenderModel(*cmdContext);      // モデル
	}

	dx12->ClearGBuffer();

	// -------------------------------------------------
	// 2. SwapChain Pass (UI / Final)
	// -------------------------------------------------
	dx12->BeginBackBufferPass();

	if (renderSys) {
		renderSys->RenderFrontSprite(*cmdContext); // 前景
	}

	// ImGui描画 
	if (uiRenderCallBack) {
		uiRenderCallBack();
	}

	dx12->ClearBackBuffer();
}
inline void GameContext::Finalize() { if(currentScene_) currentScene_->Finalize(); }

template<typename T, typename ...Args>
inline void GameContext::RegisterScene(const std::string& name, Args && ...args)
{
	// TはWorldを継承していなければいけない
	static_assert(std::is_base_of<Framework::World, T>::value,
		"RegisterScene: T must derive from World");

	using SceneType = T;
	using BaseType = Framework::World;

	// mapに追加
	std::unique_ptr<BaseType> ptr(
		new SceneType(std::forward<Args>(args)...)
	);
	ptr->SetGameContext(this); // このクラスのptrを渡す
	world_[name] = std::move(ptr);
}

inline void GameContext::ChangeScene(const std::string& name) 
{
	auto it = world_.find(name);
	if (it == world_.end()) {
		return;
	}
	currentScene_ = it->second.get();
	pendingInit_ = true;
}

}
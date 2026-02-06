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

	// ----------------------------
	// DX12 BeginFrame
	// ----------------------------
	auto* dx12 = DX12::DX12Manager::GetInstance();
	const DX12::FrameIndices idx = dx12->BeginFrame();

	auto* cmd = dx12->GetCommandContext();
	auto& frame = dx12->GetFrameResource(idx.cpu);

	// World から System を取得
	auto* renderSys = currentScene_->GetSystem<Framework::RenderSystem>();
	auto* prepSys = currentScene_->GetSystem<Framework::RenderPrepareSystem>();
	if (!renderSys || !prepSys) {
		dx12->EndFrame(idx);
		return;
	}

	// ============================================================
	// 1. GBuffer Pass（書き込み）
	// ============================================================
	dx12->TransitionGBufferToWrite();
	dx12->BeginGBufferPass();
	dx12->ClearGBuffer();

	renderSys->DrawGBufferPass(*cmd, frame, *prepSys);

	// ============================================================
	// 2. Lighting Pass（GBuffer 読み込み → BackBuffer）
	// ============================================================
	dx12->TransitionGBufferToRead();
	dx12->BeginBackBufferPass(idx.backBuffer);
	dx12->ClearBackBuffer(idx.backBuffer);

	renderSys->DrawLightingPass(*cmd, frame, *prepSys);

	// ============================================================
	// 3. Debug Overlay
	// ============================================================
#ifdef _DEBUG
	//dx12->BeginBackBufferPass(idx.backBuffer);
	//renderSys->DrawDebugPass(*cmd, frame, *prepSys);
#endif

	// ============================================================
	// 4. UI
	// ============================================================
	if (uiRenderCallBack) {
		uiRenderCallBack();
	}

	// ----------------------------
	// DX12 EndFrame
	// ----------------------------
	dx12->EndFrame(idx);
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
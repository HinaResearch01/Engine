#pragma once


namespace Tsumi::Framework {

class IActor {

public:
	 // 状態
	enum class State {
		None = -1,
		Active,
		Paused,
		Dead,
	};

	/// <summary>
	/// コンストラクタ
	/// </summary>
	IActor();

	/// <summary>
	/// デストラクタ
	/// </summary>
	virtual ~IActor();

	/// <summary>
	/// 初期化処理
	/// </summary>
	virtual void Init() = 0;

	/// <summary>
	/// 更新処理
	/// </summary>
	virtual void Update(float deltaTime);
	virtual void UpdateComponent(float deltaTime);
	virtual void UpdateActor([[maybe_unused]]float deltaTime) {};

	/// <summary>
	/// 描画処理
	/// </summary>
	void Render();

	/// <summary>
	/// コンポーネントの追加
	/// </summary>
	void AddComp();
	void AddRendComp();
	
	/// <summary>
	/// 衝突時コールバック関数
	/// </summary>
	virtual void OnCollision() {};

#pragma region Accessor
	// 状態
	State GetState() const { return state_; }
#pragma endregion 

private:
	//std::string name_ = "default";
	State state_ = IActor::State::None;
	bool isRender_ = false;
};
}

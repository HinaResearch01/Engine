#pragma once

#include "../ISystem.h"
#include "Keys.h"

#include <dinput.h>
#include <wrl.h>
#include <array>

namespace Tsumi::Framework {

class InputSystem : public ISystem {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	InputSystem(World& world);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~InputSystem() = default;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="deltaTime"></param>
	void Update(float deltaTime) override;

	// Keyboard
	bool IsTrigger(Input::Key key) const;
	bool IsHold(Input::Key key) const;
	bool IsRelease(Input::Key key) const;

	// Mouse
	bool IsTrigger(Input::Mouse button) const;
	bool IsHold(Input::Mouse button) const;
	bool IsRelease(Input::Mouse button) const;
	
	const std::pair<long, long>& GetMouseMove() const { return mouseMove_; }
	long GetWheel() const { return mouseWheel_; }

private:
	Microsoft::WRL::ComPtr<IDirectInput8> directInput_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_;

	std::array<BYTE, 256> keyState_{};
	std::array<BYTE, 256> preKeyState_{};

	DIMOUSESTATE2 mouseState_{};
	DIMOUSESTATE2 preMouseState_{};

	std::pair<long, long> mouseMove_;
	long mouseWheel_ = 0;
};

}

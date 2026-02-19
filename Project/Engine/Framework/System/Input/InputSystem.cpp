#include "InputSystem.h"
#include "Win/Win32Window.h"
#include <cassert>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace Tsumi::Framework;

InputSystem::InputSystem(World& world)
	: ISystem(world)
{}

void InputSystem::Init() {
	HRESULT result;

	// DirectInputオブジェクトの作成
	result = DirectInput8Create(
		GetModuleHandle(nullptr),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&directInput_,
		nullptr
	);
	assert(SUCCEEDED(result));

	// キーボードデバイスの作成
	result = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, nullptr);
	assert(SUCCEEDED(result));

	// 入力データ形式のセット
	result = keyboard_->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	// 排他制御レベルのセット
	result = keyboard_->SetCooperativeLevel(
		Tsumi::Win32::Win32Window::GetInstance()->GetHWND(),
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY
	);
	assert(SUCCEEDED(result));

	// マウスデバイスの作成
	result = directInput_->CreateDevice(GUID_SysMouse, &mouse_, nullptr);
	assert(SUCCEEDED(result));

	// 入力データ形式のセット
	result = mouse_->SetDataFormat(&c_dfDIMouse2);
	assert(SUCCEEDED(result));

	// 排他制御レベルのセット
	result = mouse_->SetCooperativeLevel(
		Tsumi::Win32::Win32Window::GetInstance()->GetHWND(),
		DISCL_FOREGROUND | DISCL_NONEXCLUSIVE
	);
	assert(SUCCEEDED(result));
}

void InputSystem::Update(float) {
	// キーボード情報の取得開始
	keyboard_->Acquire();
	preKeyState_ = keyState_;
	keyboard_->GetDeviceState(sizeof(keyState_), keyState_.data());

	// マウス情報の取得開始
	mouse_->Acquire();
	preMouseState_ = mouseState_;
	mouse_->GetDeviceState(sizeof(mouseState_), &mouseState_);

	mouseMove_ = { mouseState_.lX, mouseState_.lY };
	mouseWheel_ = mouseState_.lZ;
}

bool InputSystem::IsTrigger(Input::Key key) const {
	return (keyState_[static_cast<int>(key)] & 0x80) && !(preKeyState_[static_cast<int>(key)] & 0x80);
}

bool InputSystem::IsHold(Input::Key key) const {
	return (keyState_[static_cast<int>(key)] & 0x80);
}

bool InputSystem::IsRelease(Input::Key key) const {
	return !(keyState_[static_cast<int>(key)] & 0x80) && (preKeyState_[static_cast<int>(key)] & 0x80);
}

bool InputSystem::IsTrigger(Input::Mouse button) const {
	return (mouseState_.rgbButtons[static_cast<int>(button)] & 0x80) && !(preMouseState_.rgbButtons[static_cast<int>(button)] & 0x80);
}

bool InputSystem::IsHold(Input::Mouse button) const {
	return (mouseState_.rgbButtons[static_cast<int>(button)] & 0x80);
}

bool InputSystem::IsRelease(Input::Mouse button) const {
	return !(mouseState_.rgbButtons[static_cast<int>(button)] & 0x80) && (preMouseState_.rgbButtons[static_cast<int>(button)] & 0x80);
}

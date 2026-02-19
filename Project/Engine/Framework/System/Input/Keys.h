#pragma once

#include <dinput.h>

namespace Tsumi::Framework::Input {

enum class Key {
	/* アルファベット */
	A = DIK_A, B = DIK_B, C = DIK_C, D = DIK_D, E = DIK_E, F = DIK_F, G = DIK_G,
	H = DIK_H, I = DIK_I, J = DIK_J, K = DIK_K, L = DIK_L, M = DIK_M, N = DIK_N,
	O = DIK_O, P = DIK_P, Q = DIK_Q, R = DIK_R, S = DIK_S, T = DIK_T, U = DIK_U,
	V = DIK_V, W = DIK_W, X = DIK_X, Y = DIK_Y, Z = DIK_Z,

	/* 数字 */
	K0 = DIK_0, K1 = DIK_1, K2 = DIK_2, K3 = DIK_3, K4 = DIK_4,
	K5 = DIK_5, K6 = DIK_6, K7 = DIK_7, K8 = DIK_8, K9 = DIK_9,

	/* 矢印キー */
	UP = DIK_UP, DOWN = DIK_DOWN, LEFT = DIK_LEFT, RIGHT = DIK_RIGHT,

	/* 機能キー */
	SPACE = DIK_SPACE, ENTER = DIK_RETURN, ESCAPE = DIK_ESCAPE,
	LSHIFT = DIK_LSHIFT, RSHIFT = DIK_RSHIFT,
	LCTRL = DIK_LCONTROL, RCTRL = DIK_RCONTROL,
	LALT = DIK_LALT, RALT = DIK_RALT,
	TAB = DIK_TAB, BACKSPACE = DIK_BACK,

	/* ファンクションキー */
	F1 = DIK_F1, F2 = DIK_F2, F3 = DIK_F3, F4 = DIK_F4, F5 = DIK_F5,
	F6 = DIK_F6, F7 = DIK_F7, F8 = DIK_F8, F9 = DIK_F9, F10 = DIK_F10,
	F11 = DIK_F11, F12 = DIK_F12,
};

enum class Mouse {
	LEFT,
	RIGHT,
	MIDDLE,
};

}

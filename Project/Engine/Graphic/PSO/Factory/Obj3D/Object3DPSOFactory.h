#pragma once

#include "../../PSOUtil.h"

namespace Tsumi::Graphic{

// 前方宣言
class PSOLibrary;

/* Object3DのPSOの構築クラス */
class Object3DPSOFactory {

public:
	/// <summary>
	/// 構築
	/// </summary>
	static void Build(PSOLibrary& lib);
};

}
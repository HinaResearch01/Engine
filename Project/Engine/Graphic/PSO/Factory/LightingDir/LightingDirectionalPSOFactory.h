#pragma once

#include "../../PSOUtil.h"

namespace Tsumi::Graphic {

// 前方宣言
class PSOLibrary;

/* LightingDirのPSOの構築クラス */
class LightingDirectionalPSOFactory {

public:
	/// <summary>
	/// 構築
	/// </summary>
	static void Build(PSOLibrary& lib);
};

}
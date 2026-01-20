#pragma once

#include "../../PSOUtil.h"

namespace Tsumi::Graphic {

// 前方宣言
class PSOLibrary;

/* ShadowDirのPSOの構築クラス */
class ShadowCasterDirectionalFactory {

public:
	/// <summary>
	/// 構築
	/// </summary>
	static void Build(PSOLibrary& lib);
};

}
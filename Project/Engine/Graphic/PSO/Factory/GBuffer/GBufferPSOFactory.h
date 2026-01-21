#pragma once

#include "../../PSOUtil.h"

namespace Tsumi::Graphic {

// 前方宣言
class PSOLibrary;

/* GBufferのPSOの構築クラス */
class GBufferPSOFactory {

public:
	/// <summary>
	/// 構築
	/// </summary>
	static void Build(PSOLibrary& lib);
};

}
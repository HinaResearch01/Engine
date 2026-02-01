#pragma once

#include <cstdint>
#include "DX12/Cmd/CommandContext.h"
#include "FrameBindState.h"
#include "FrameUploadArena.h"
#include "DX12/Desc/TransientDescAllocator.h"

namespace Tsumi::DX12 {

/*  */
class FrameContext {

public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	FrameContext() = default;

	/// <summary>
	/// デストラクタ
	/// </summary>
	~FrameContext() = default;

	/// <summary>
	/// フレーム開始時処理
	/// </summary>
	void Begin(DX12::CommandContext& cmd) {
		upload.Begin();
		transDescAlloc.Begin();
		bind.Begin(cmd);
	}

	/// <summary>
	/// リセット処理
	/// </summary>
	void Reset() {
		bind.Reset();
	}

public:
	FrameUploadArena upload;
	TransientDescAllocator transDescAlloc;
	FrameBindState bind;

	uint64_t fenceValue = 0;
};

}
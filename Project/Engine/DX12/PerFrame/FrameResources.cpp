#include "FrameResources.h"
#include "DX12/Cmd/CommandContext.h"

using namespace Tsumi::DX12;

void FrameResources::Begin(DX12::CommandContext& cmd)
{
	upload.Begin();
	transDescAlloc.Begin();
	bind.Begin(cmd.GetList());
}

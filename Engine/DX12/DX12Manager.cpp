#include "DX12Manager.h"

using namespace Tsumi::DX12;

DX12Manager::DX12Manager()
{
	dx12Device_ = std::make_unique<DX12Device>();
	cmdContext_ = std::make_unique<CommandContext>(this);
	swapChain_ = std::make_unique<SwapChain> (this);
	framebuf_ = std::make_unique<Framebuffer>(this);
}

void DX12Manager::Init()
{
	try {
		DX_CALL(dx12Device_->Create());
		if(cmdContext_) cmdContext_->SetFrameCount(bufferCount_);
		DX_CALL(cmdContext_->Create());
		DX_CALL(swapChain_->Create());
		DX_CALL(framebuf_->Init());
	}
	catch (const DxException& e) {
		// Visual Studio の出力ウィンドウにメッセージを出す
		OutputDebugStringA(e.what());

		// ユーザーに通知して終了
		MessageBoxA(nullptr, e.what(), "Fatal DirectX Error", MB_OK | MB_ICONERROR);
		std::terminate();
	}
}

void DX12Manager::PreDraw4PE()
{
}

void DX12Manager::PostDraw4PE()
{	 
}	 
	 
void DX12Manager::PreDraw4SC()
{	 
}	 
	 
void DX12Manager::PostDraw4SC()
{
}

void DX12Manager::OnFinalize()
{
}
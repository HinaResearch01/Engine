#include "SwapChain.h"
#include "../../Core/App/Application.h"

using namespace Tsumi::DX12;

void SwapChain::Create()
{
    HWND hwnd = Application::GetInstance()->GetHwnd();

    // 画面のクライアント領域
    desc_.Width = Application::GetInstance()->GetAppDesc().windowWidth;
    desc_.Height = Application::GetInstance()->GetAppDesc().windowHeight; 

    // 色の形式
    desc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    // マルチサンプルしない
    desc_.SampleDesc.Count = 1;
    // 描画のターゲットとして利用する
    desc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; 
    // ダブルバッファ
    desc_.BufferCount = 2;
    // モニタにうつしたら、中身を破棄
    desc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; 


    // コマンドキューに設定を渡して生成

}

void Tsumi::DX12::SwapChain::Present(UINT syncInterval, UINT flags)
{
    if (swapChain_) {
        swapChain_->Present(syncInterval, flags);
    }
}

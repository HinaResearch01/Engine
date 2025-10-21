#include "GraphicsDevice.h"

using namespace Tsumi::DX12;

GraphicsDevice::~GraphicsDevice()
{
    if (debugController_) {
        debugController_.Reset();
    }
    if (infoQueue_) {
        infoQueue_.Reset();
    }
    device_.Reset();
    factory_.Reset();
}

std::unique_ptr<GraphicsDevice> GraphicsDevice::Create(bool enableDebug)
{
    auto mgr = std::unique_ptr<GraphicsDevice>(new GraphicsDevice());
    if (!mgr->Init(enableDebug)) {
        return nullptr;
    }
    return mgr;
}

void GraphicsDevice::SetupInfoQueue()
{
    if (!infoQueue_) return;

    // 例: D3D12_MESSAGE_SEVERITY_CORRUPTION / ERROR でブレーク
    infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

    // 任意のメッセージを無視したい場合はフィルタを設定する (例を簡単に)
    D3D12_INFO_QUEUE_FILTER filter{};
    // ここで filter.AllowList / DenyList を設定できる
    infoQueue_->AddStorageFilterEntries(&filter);
}

bool GraphicsDevice::Init(bool enableDebug)
{
    HRESULT hr = S_OK;

    // デバッグレイヤーの有効化 (存在すれば)
    if (enableDebug) {
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_)))) {
            debugController_->EnableDebugLayer();
        }
        else {
            // Debug レイヤが存在しない環境もあるため致命扱いにしない
            debugController_.Reset();
        }
    }

    // DXGI Factory 作成
    UINT dxgiFlags = 0;
    if (enableDebug && debugController_) {
        dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    hr = CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&factory_));
    if (FAILED(hr) || !factory_) {
        return false;
    }

    // D3D12 Device 作成 (まずは既定アダプタで試す)
    hr = D3D12CreateDevice(
        nullptr,                    // adapter (nullptr -> WARP/既定 アダプタが選ばれる)
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device_));
    if (FAILED(hr) || !device_) {
        // 失敗した場合はアダプタ列挙して試す（簡易フォールバック）
        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; factory_->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            // ソフトウェアアダプタはスキップ
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                adapter.Reset();
                continue;
            }
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)))) {
                break;
            }
            adapter.Reset();
        }
    }

    if (!device_) {
        return false;
    }

    // InfoQueue を取得して任意のフィルタ設定ができるようにする
    // QueryInterface を使う（存在しない環境もあり得るので失敗は無視）
    device_->QueryInterface(IID_PPV_ARGS(&infoQueue_));

    return true;
}

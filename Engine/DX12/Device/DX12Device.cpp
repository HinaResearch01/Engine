#include "DX12Device.h"
#include "../../Utils/Log/UtilsLog.h"
#include <format>
#include <cassert>
#include <wrl.h>
#include <array>

using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;

DX12Device::~DX12Device()
{
    infoQueue_.Reset();
    device_.Reset();
    useAdapter_.Reset();
    factory_.Reset();
    debugController_.Reset();
}

HRESULT DX12Device::Create()
{
    HRESULT hr = S_OK;

    // デバッグレイヤーは必須ではないので失敗しても継続可能
    hr = CreateDebugLayer();
    if (FAILED(hr)) {
        // ログは残すが、継続する（環境によっては無視できる）
        Utils::Log(std::format(L"Warning: CreateDebugLayer failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
    }

    hr = CreateFactoryAndAdapter();
    if (FAILED(hr)) {
        Utils::Log(std::format(L"Error: CreateFactoryAndAdapter failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return hr; // ここは致命的
    }

    hr = CreateDevice();
    if (FAILED(hr)) {
        Utils::Log(std::format(L"Error: CreateDevice failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
        return hr; // ここも致命的
    }

    hr = DebugErrorInfoQueue();
    if (FAILED(hr)) {
        // InfoQueue の取得失敗は必ずしも致命的ではないがログに残す
        Utils::Log(std::format(L"Warning: DebugErrorInfoQueue failed (hr=0x{:08X})\n", static_cast<unsigned>(hr)));
    }

    return S_OK;
}

HRESULT Tsumi::DX12::DX12Device::CreateDebugLayer()
{
#ifdef _DEBUG
    ComPtr<ID3D12Debug1> debug;
    HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
    if (SUCCEEDED(hr) && debug) {
        debug->EnableDebugLayer();

        // GPU ベース検証を有効にするには ID3D12Debug1
        ComPtr<ID3D12Debug1> debug1;
        if (SUCCEEDED(debug.As(&debug1)) && debug1) {
            debug1->SetEnableGPUBasedValidation(TRUE);
        }

        // Optionally keep a reference to the debug controller
        debugController_ = debug;
        return S_OK;
    }

    // デバッグインターフェイスが取得できない環境もある（non-fatal）
    return hr;
#else
    // リリースビルドでは単に成功を返す
    return S_OK;
#endif // _DEBUG
}

HRESULT Tsumi::DX12::DX12Device::CreateFactoryAndAdapter()
{
    factory_.Reset();
#ifdef _DEBUG
    // デバッグファクトリを使いたければ CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, ...) を検討
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory_));
#else
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory_));
#endif
    if (FAILED(hr) || !factory_) {
        return hr;
    }

    useAdapter_.Reset();
    // 高性能順にアダプタを列挙してハードウェアアダプタを選ぶ
    for (UINT i = 0; ; ++i) {
        ComPtr<IDXGIAdapter4> adapter;
        hr = factory_->EnumAdapterByGpuPreference(
            i,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(&adapter)
        );

        if (hr == DXGI_ERROR_NOT_FOUND) {
            // もうアダプタがない
            break;
        }
        if (FAILED(hr)) {
            // 列挙失敗は次へ（ただし多く失敗するなら最終的にエラーにする）
            continue;
        }

        DXGI_ADAPTER_DESC3 desc{};
        if (FAILED(adapter->GetDesc3(&desc))) {
            continue;
        }

        // ソフトウェアアダプタはスキップ
        if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
            continue;
        }

        // 採用
        useAdapter_ = adapter;
        Utils::Log(std::format(L"USE Adapter: {}\n", desc.Description));
        break;
    }

    if (!useAdapter_) {
        // 適切なアダプタが見つからなかった
        return E_FAIL;
    }

    return S_OK;
}

HRESULT Tsumi::DX12::DX12Device::CreateDevice()
{
    std::array<D3D_FEATURE_LEVEL, 3> featureLevels = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0
    };

    std::array<std::wstring, 3> featureLevelStrings = {
        L"12.2", L"12.1", L"12.0"
    };

    if (!useAdapter_) {
        return E_POINTER;
    }

    device_.Reset();
    HRESULT hr = E_FAIL;
    for (size_t i = 0; i < featureLevels.size(); ++i) {
        hr = D3D12CreateDevice(useAdapter_.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
        if (SUCCEEDED(hr) && device_) {
            Utils::Log(std::format(L"FeatureLevel : {}\n", featureLevelStrings[i]));
            break;
        }
    }

    if (FAILED(hr) || !device_) {
        return hr == E_FAIL ? E_FAIL : hr;
    }

    Utils::Log(L"Complete create D3D12Device!!!\n");
    return S_OK;
}

HRESULT Tsumi::DX12::DX12Device::DebugErrorInfoQueue()
{
#ifdef _DEBUG
    if (!device_) {
        return E_POINTER;
    }

    ComPtr<ID3D12InfoQueue> infoQueue;
    HRESULT hr = device_.As(&infoQueue);
    if (FAILED(hr) || !infoQueue) {
        return hr;
    }

    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

    D3D12_MESSAGE_ID denyIds[] = {
        D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
    };

    D3D12_MESSAGE_SEVERITY severities[] = {
        D3D12_MESSAGE_SEVERITY_INFO
    };

    D3D12_INFO_QUEUE_FILTER localFilter{};
    localFilter.DenyList.NumIDs = _countof(denyIds);
    localFilter.DenyList.pIDList = denyIds;
    localFilter.DenyList.NumSeverities = _countof(severities);
    localFilter.DenyList.pSeverityList = severities;

    infoQueue->PushStorageFilter(&localFilter);

    // メンバとして保持する（必要なら）。ComPtr により自動管理。
    infoQueue_ = infoQueue;

    return S_OK;
#else
    return S_OK;
#endif // _DEBUG
}


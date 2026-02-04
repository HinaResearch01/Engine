#include "DX12Device.h"
#include "Utils/Logger/Logger.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;
using namespace Tsumi::DX12;

DX12Device::~DX12Device()
{
#ifdef _DEBUG
	infoQueue_.Reset();
#endif
	device_.Reset();
	useAdapter_.Reset();
	factory_.Reset();
}

HRESULT DX12Device::Create()
{
	HRESULT hr;

#ifdef _DEBUG
	hr = CreateDebugLayer();
	if (FAILED(hr)) {
		Utils::Logger::Warn("CreateDebugLayer failed (hr=0x{:08X})\n", (unsigned)hr);
	}
#endif

	hr = CreateFactoryAndAdapter();
	if (FAILED(hr)) {
		Utils::Logger::Error("CreateFactoryAndAdapter failed (hr=0x{:08X})\n", (unsigned)hr);
		return hr;
	}

	hr = CreateDevice();
	if (FAILED(hr)) {
		Utils::Logger::Error("CreateDevice failed (hr=0x{:08X})\n", (unsigned)hr);
		return hr;
	}

#ifdef _DEBUG
	hr = DebugErrorInfoQueue();
	if (FAILED(hr)) {
		Utils::Logger::Warn("DebugErrorInfoQueue failed (hr=0x{:08X})\n", (unsigned)hr);
	}
#endif

	Utils::Logger::Info("DX12Device::Create SUCCESS\n");
	return S_OK;
}

HRESULT DX12Device::CreateDebugLayer()
{
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debug;
	HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (SUCCEEDED(hr) && debug) {
		debug->EnableDebugLayer();
		Utils::Logger::Info("D3D12 DebugLayer enabled\n");
		return S_OK;
	}
	return hr;
#else
	return S_OK;
#endif
}

HRESULT DX12Device::CreateFactoryAndAdapter()
{
	factory_.Reset();
	useAdapter_.Reset();

#ifdef _DEBUG
	HRESULT hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,
									IID_PPV_ARGS(&factory_));
#else
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory_));
#endif
	if (FAILED(hr) || !factory_) {
		Utils::Logger::Error("CreateDXGIFactory failed (hr=0x{:08X})\n", (unsigned)hr);
		return hr;
	}

	Utils::Logger::Info("DXGI Factory created\n");

	for (UINT i = 0;; ++i) {
		ComPtr<IDXGIAdapter1> adapter;
		hr = factory_->EnumAdapters1(i, &adapter);
		if (hr == DXGI_ERROR_NOT_FOUND) {
			Utils::Logger::Error("No hardware adapter found\n");
			break;
		}
		if (FAILED(hr)) {
			Utils::Logger::Warn("EnumAdapters1 failed (i={})\n", i);
			continue;
		}

		DXGI_ADAPTER_DESC1 desc{};
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			Utils::Logger::Info("Skip software adapter\n");
			continue;
		}

		Utils::Logger::Info("Use Adapter selected\n");
		adapter.As(&useAdapter_);
		return S_OK;
	}

	return E_FAIL;
}

HRESULT DX12Device::CreateDevice()
{
	if (!useAdapter_) {
		Utils::Logger::Error("CreateDevice: adapter is null\n");
		return E_POINTER;
	}

	static constexpr D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0
	};

	device_.Reset();

	for (auto fl : levels) {
		Utils::Logger::Info("Try D3D12CreateDevice (FL={})\n", (int)fl);

		HRESULT hr = D3D12CreateDevice(
			useAdapter_.Get(),
			fl,
			IID_PPV_ARGS(&device_)
		);

		if (SUCCEEDED(hr) && device_) {
			Utils::Logger::Info("D3D12Device created (FL={})\n", (int)fl);
			return S_OK;
		}

		Utils::Logger::Warn(
			"D3D12CreateDevice failed (FL={}, hr=0x{:08X})\n",
			(int)fl, (unsigned)hr
		);
	}

	Utils::Logger::Error("All feature levels failed in D3D12CreateDevice\n");
	return E_FAIL;
}

HRESULT DX12Device::DebugErrorInfoQueue()
{
#ifdef _DEBUG
	if (!device_) return E_POINTER;

	ComPtr<ID3D12InfoQueue> q;
	HRESULT hr = device_.As(&q);
	if (FAILED(hr) || !q) {
		Utils::Logger::Warn("ID3D12InfoQueue not available\n");
		return hr;
	}

	q->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	q->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

	infoQueue_ = q;
	Utils::Logger::Info("D3D12 InfoQueue enabled\n");
#endif
	return S_OK;
}

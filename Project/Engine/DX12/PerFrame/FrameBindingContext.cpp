#include "FrameBindingContext.h"
#include "DX12/DX12Manager.h"

using namespace Tsumi::DX12;

void FrameBindingContext::BeginFrame(CommandContext& cmd, ID3D12DescriptorHeap* globalCbvSrvUavHeap)
{
	auto* list = cmd.GetList();
	if (!list || !globalCbvSrvUavHeap) return;

	ID3D12DescriptorHeap* heaps[] = { globalCbvSrvUavHeap };
	list->SetDescriptorHeaps(1, heaps);
	heapsSet_ = true;
}

D3D12_GPU_DESCRIPTOR_HANDLE FrameBindingContext::BuildSrvTable(PerFrameResource& fr, std::span<const DescriptorHandle> persistentHandles)
{
	D3D12_GPU_DESCRIPTOR_HANDLE nullGpu{ 0 };
	if (persistentHandles.empty()) return nullGpu;

	// source は “CPU handle” でコピーする（同じGlobalHeap内でもOK）
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srcCpu;
	srcCpu.reserve(persistentHandles.size());
	for (auto& h : persistentHandles) {
		srcCpu.push_back(h.cpu);
	}

	return fr.GetTableBuilder().BuildTable(srcCpu);
}

void FrameBindingContext::BindTable(CommandContext& cmd, uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE tableGpu) const
{
	if (!heapsSet_ || tableGpu.ptr == 0) return;
	auto* list = cmd.GetList();
	if (!list) return;

	list->SetGraphicsRootDescriptorTable(rootIndex, tableGpu);
}

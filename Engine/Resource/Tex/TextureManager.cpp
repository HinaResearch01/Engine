#include "TextureManager.h"
#include "DX12/DX12Manager.h"
#include "DX12/Desc/DescriptorAllocator.h"
#include "Utils/Logger/UtilsLog.h"
#include <DirectXTex.h>

using namespace Tsumi::Resource;
using namespace Tsumi::DX12;
using Microsoft::WRL::ComPtr;
using namespace DirectX;

static std::wstring ToWString(const std::string& s) {
	return std::wstring(s.begin(), s.end());
}

TextureManager::TextureManager()
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
}

void TextureManager::Emplace(const std::string& name, Texture* tex)
{
    textures_.emplace(name, std::move(tex));
}

void TextureManager::UnloadAll()
{
    auto* allocator = dx12Mgr_->GetPersistentDescAlloc();
    if (allocator) {
        uint32_t frameIndex = dx12Mgr_->GetFrameSync()->GetFrameIndex();
        for (auto& [name, tex] : textures_) {
            if (tex && tex->srvDesc.valid()) {
                allocator->DeferFree(tex->srvDesc, frameIndex);
            }
        }
    }

    textures_.clear();
}

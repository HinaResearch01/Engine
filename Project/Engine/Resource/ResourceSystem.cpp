#include "ResourceSystem.h"
#include "DX12/DX12Manager.h"
#include "Resource/Mesh/MeshManager.h"
#include "Resource/Tex/TextureManager.h"
#include "Resource/CB/FrameCBManager.h"
#include <DirectXTex.h>

using namespace Tsumi::Resource;

ResourceSystem::ResourceSystem()
{
	dx12Mgr_ = Tsumi::DX12::DX12Manager::GetInstance();
	meshMgr_ = std::make_unique<MeshManager>();
	texMgr_ = std::make_unique<TextureManager>();
	cbMgr_ = std::make_unique<FrameCBManager>();
}

ResourceSystem::~ResourceSystem() = default;

void ResourceSystem::Init()
{
	cbMgr_->Init();

	// デフォルト "White" テクスチャの生成
	{
		DirectX::ScratchImage image;
		image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);
		uint8_t* pixels = image.GetPixels();
		pixels[0] = 0xFF; 
		pixels[1] = 0xFF; 
		pixels[2] = 0xFF; 
		pixels[3] = 0xFF;
		
		// 登録 (viewFormatはデフォルト)
		texMgr_->RegisterTexture("White", image, DXGI_FORMAT_R8G8B8A8_UNORM);
	}
}

void ResourceSystem::BeginFrame(uint32_t frameIndex)
{
	if (cbMgr_) cbMgr_->BeginFrame(frameIndex);
}

void ResourceSystem::SceneReset()
{
	meshMgr_->UnloadAll();
	texMgr_->UnloadAll();
}

void ResourceSystem::Finalize()
{
	if (cbMgr_) cbMgr_->Finalize();
	texMgr_.reset();
	meshMgr_.reset();
}

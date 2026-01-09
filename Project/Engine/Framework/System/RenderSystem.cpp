#include "RenderSystem.h"
#include "DX12/DX12Manager.h"
#include "Resource/ResourceSystem.h"
#include "Graphic/PSO/PSOLibrary.h"
#include "Graphic/RootSigs/RootSignatureLibrary.h"

using namespace Tsumi::Framework;

RenderSystem::RenderSystem(World& world)
	: world_(world)
{
	dx12Mgr_ = DX12::DX12Manager::GetInstance();
	resourceSys_ = Resource::ResourceSystem::GetInstance();
	psoLib_ = Graphic::PSOLibrary::GetInstance();
	rootSigLib_ = Graphic::RootSignatureLibrary::GetInstance();
}

void RenderSystem::Update(float)
{
}

void RenderSystem::RenderBackSprite(DX12::CommandContext& cmd)
{
	cmd;
}

void RenderSystem::RenderModel(DX12::CommandContext& cmd)
{
	cmd;
}

void RenderSystem::RenderFrontSprite(DX12::CommandContext& cmd)
{
	cmd;
}

#include "World.h"
#include "DX12/DX12Manager.h"
#include "DX12/Framebuf/Framebuffer.h"

float Tsumi::Framework::World::GetAspectRatio() const
{
	auto* dx12 = Tsumi::DX12::DX12Manager::GetInstance();
	auto* fb = dx12 ? dx12->GetFramebuffer() : nullptr;
	float w = fb ? (float)fb->GetWidth() : 1280.0f;
	float h = fb ? (float)fb->GetHeight() : 720.0f;
	return (h > 0.0f) ? (w / h) : (1280.0f / 720.0f);
}

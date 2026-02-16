#include "DebugUISystem.h"
#include <imgui.h>
#include <imgui_internal.h>

using namespace Tsumi::Framework;

DebugUISystem::DebugUISystem(World& world)
	: world_(world)
{}

void DebugUISystem::Update(float)
{
}

#include "ISystem.h"
#include "Framework/World/World.h"

Tsumi::Framework::ISystem::ISystem(World& world) 
	: world_(world);
{}

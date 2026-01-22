#pragma once

#include <cstdint>
#include <vector>
#include "Framework/Str/LightGpuPackets.h"

namespace Tsumi::Framework {

struct LightPacket
{
	GpuDirectionalLightCB dirCB{};
	std::vector<GpuPointLightCB> pointCB;
	std::vector<GpuSpotLightCB> spotCB;
};

}

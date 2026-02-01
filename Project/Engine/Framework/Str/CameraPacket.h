#pragma once

#include <cstdint>
#include <vector>
#include "Framework/Str/CameraGpuStructure.h"

namespace Tsumi::Framework {

struct CameraPacket {
	GpuCameraMatricesCB camMatCB;
	GpuCameraParameterCB camParamCB;
};

}

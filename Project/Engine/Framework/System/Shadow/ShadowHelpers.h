#pragma once
#include "../../Context/ShadowContext.h"
#include "../../Context/CameraContext.h"
#include "../../../Math/TMath.h"
#include <array>
#include <cmath>

namespace Tsumi {
namespace Framework {

// Shadow System Internal Helpers
namespace ShadowDetail {

struct ShadowMath {
	static Math::Vec3f NormalizeSafe(const Math::Vec3f& v, const Math::Vec3f& fallback);

	static void GetFrustumCornersWS(
		const Math::Mat4x4& invViewProj,
		float ndcNearZ, float ndcFarZ,
		Math::Vec3f outCorners[8]
	);

	static Math::Vec3f Average8(const Math::Vec3f p[8]);

};

}
}
}

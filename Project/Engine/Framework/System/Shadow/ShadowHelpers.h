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

using namespace tme;

struct ShadowMath {
	static math::Vec3f NormalizeSafe(const math::Vec3f& v, const math::Vec3f& fallback);

	static void GetFrustumCornersWS(
		const math::Mat4x4& invViewProj,
		float ndcNearZ, float ndcFarZ,
		math::Vec3f outCorners[8]
	);

	static math::Vec3f Average8(const math::Vec3f p[8]);

};

}
}
}

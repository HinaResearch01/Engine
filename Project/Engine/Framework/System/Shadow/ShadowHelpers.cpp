#include "ShadowHelpers.h"
#include <algorithm>
#include <cmath>
#undef min
#undef max

namespace Tsumi {
namespace Framework {
namespace ShadowDetail {

using namespace tme;

	// =========================================================
	// ShadowMath
	// =========================================================

	math::Vec3f ShadowMath::NormalizeSafe(const math::Vec3f& v, const math::Vec3f& fallback)
	{
		const float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
		if (len2 <= 1e-12f) return fallback;
		const float invLen = 1.0f / std::sqrt(len2);
		return { v.x * invLen, v.y * invLen, v.z * invLen };
	}

	void ShadowMath::GetFrustumCornersWS(
		const math::Mat4x4& invViewProj,
		float ndcNearZ, float ndcFarZ,
		math::Vec3f outCorners[8]
	)
	{
		// D3D NDC: x,y [-1..1], z [0..1]
		const float xs[2] = { -1.0f, 1.0f };
		const float ys[2] = { -1.0f, 1.0f };

		int idx = 0;
		for (int y = 0; y < 2; ++y)
			for (int x = 0; x < 2; ++x)
			{
				// near
				{
					math::Vec4f c = { xs[x], ys[y], ndcNearZ, 1.0f };
					math::Vec4f w = invViewProj * c;
					const float iw = (std::abs(w.w) > 1e-6f) ? (1.0f / w.w) : 1.0f;
					outCorners[idx++] = { w.x * iw, w.y * iw, w.z * iw };
				}
			}
		for (int y = 0; y < 2; ++y)
			for (int x = 0; x < 2; ++x)
			{
				// far
				{
					math::Vec4f c = { xs[x], ys[y], ndcFarZ, 1.0f };
					math::Vec4f w = invViewProj * c;
					const float iw = (std::abs(w.w) > 1e-6f) ? (1.0f / w.w) : 1.0f;
					outCorners[idx++] = { w.x * iw, w.y * iw, w.z * iw };
				}
			}
	}

	math::Vec3f ShadowMath::Average8(const math::Vec3f p[8])
	{
		math::Vec3f s{ 0,0,0 };
		for (int i = 0; i < 8; ++i) { s.x += p[i].x; s.y += p[i].y; s.z += p[i].z; }
		return { s.x / 8.0f, s.y / 8.0f, s.z / 8.0f };
	}

}
}
}

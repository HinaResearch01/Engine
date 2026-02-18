#include "ShadowHelpers.h"
#include <algorithm>
#include <cmath>
#undef min
#undef max

namespace Tsumi {
namespace Framework {
namespace ShadowDetail {

	// =========================================================
	// ShadowMath
	// =========================================================

	Math::Vec3f ShadowMath::NormalizeSafe(const Math::Vec3f& v, const Math::Vec3f& fallback)
	{
		const float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
		if (len2 <= 1e-12f) return fallback;
		const float invLen = 1.0f / std::sqrt(len2);
		return { v.x * invLen, v.y * invLen, v.z * invLen };
	}

	void ShadowMath::GetFrustumCornersWS(
		const Math::Mat4x4& invViewProj,
		float ndcNearZ, float ndcFarZ,
		Math::Vec3f outCorners[8]
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
					Math::Vec4f c = { xs[x], ys[y], ndcNearZ, 1.0f };
					Math::Vec4f w = invViewProj * c;
					const float iw = (std::abs(w.w) > 1e-6f) ? (1.0f / w.w) : 1.0f;
					outCorners[idx++] = { w.x * iw, w.y * iw, w.z * iw };
				}
			}
		for (int y = 0; y < 2; ++y)
			for (int x = 0; x < 2; ++x)
			{
				// far
				{
					Math::Vec4f c = { xs[x], ys[y], ndcFarZ, 1.0f };
					Math::Vec4f w = invViewProj * c;
					const float iw = (std::abs(w.w) > 1e-6f) ? (1.0f / w.w) : 1.0f;
					outCorners[idx++] = { w.x * iw, w.y * iw, w.z * iw };
				}
			}
	}

	Math::Vec3f ShadowMath::Average8(const Math::Vec3f p[8])
	{
		Math::Vec3f s{ 0,0,0 };
		for (int i = 0; i < 8; ++i) { s.x += p[i].x; s.y += p[i].y; s.z += p[i].z; }
		return { s.x / 8.0f, s.y / 8.0f, s.z / 8.0f };
	}

	void ShadowMath::CalculateBoundingSphere(
		const Math::Vec3f corners[8],
		Math::Vec3f& outCenter,
		float& outRadius
	)
	{
		// simple approximate center
		outCenter = Average8(corners);

		float maxDist2 = 0.0f;
		for (int i = 0; i < 8; ++i)
		{
			Math::Vec3f d = corners[i] - outCenter;
			float dist2 = d.x * d.x + d.y * d.y + d.z * d.z;
			if (dist2 > maxDist2) maxDist2 = dist2;
		}
		outRadius = std::sqrt(maxDist2);
	}

	// =========================================================
	// CSMGenerator
	// =========================================================

	void CSMGenerator::Update(
		const CameraContext& camera,
		const Math::Vec3f& lightDir,
		const ShadowCascadeConfig& config,
		ShadowContext& outCtx
	)
	{
		outCtx.enabled = true;
		outCtx.shadowMapSize = (uint32_t)config.shadowMapSize;
		outCtx.cascadeCount = config.cascadeCount;

		// 1. Calculate Split Distances
		// Use Camera Far for split distribution
		float f = camera.farPlane;

		CalcSplits(camera.nearPlane, f, config.lambda);

		// 2. Build Cascades
		for (uint32_t i = 0; i < config.cascadeCount; ++i)
		{
			outCtx.splitFar[i] = splits_[i + 1];
			BuildCascade(i, camera, lightDir, config, outCtx);
		}
	}

	void CSMGenerator::CalcSplits(float nearZ, float farZ, float lambda)
	{
		splits_[0] = nearZ;
		for (int i = 1; i <= 4; ++i)
		{
			const float p = (float)i / 4.0f;
			const float logSplit = nearZ * std::pow(farZ / nearZ, p);
			const float uniSplit = nearZ + (farZ - nearZ) * p;
			splits_[i] = lambda * logSplit + (1.0f - lambda) * uniSplit;
		}
	}

	void CSMGenerator::BuildCascade(
		uint32_t index,
		const CameraContext& camera,
		const Math::Vec3f& lightDir,
		const ShadowCascadeConfig& config,
		ShadowContext& outCtx
	)
	{
		float cn = splits_[index];
		float cf = splits_[index + 1];

		// 1. Get Frustum Corners for this slice
		const Math::Mat4x4 cascadeProj =
			Math::Func::MAT4x4::PerspectiveFovMatrix(
				camera.fovY, camera.aspectRatio, cn, cf
			);

		const Math::Mat4x4 cascadeVP = camera.view * cascadeProj;
		const Math::Mat4x4 invCascadeVP = cascadeVP.Inverse();

		Math::Vec3f cornersWS[8]{};
		ShadowMath::GetFrustumCornersWS(invCascadeVP, 0.0f, 1.0f, cornersWS);

		// 2. Calculate Bounding Sphere (Center & Radius)
		// Using a bounding sphere ensures the projection size doesn't change with camera rotation.
		Math::Vec3f sphereCenterWS;
		float sphereRadius;
		ShadowMath::CalculateBoundingSphere(cornersWS, sphereCenterWS, sphereRadius);

		// 3. Setup Light View (Initial)
		Math::Vec3f up{ 0,1,0 };
		if (std::abs(lightDir.y) > 0.99f) up = { 1,0,0 };
		
		// To avoid swimming, we need to snap the movement to texel increments.
		// First, get the light view matrix looking at the sphere center.
		// Pull back light position to ensure everything is in front of near plane.
		// Note: The distance doesn't affect orthographic projection scale, only Z values.
		float dist = sphereRadius + config.lightFarZ; // Sufficient padding
		Math::Vec3f eyeWS = sphereCenterWS - lightDir * dist;
		
		Math::Mat4x4 lightView = Math::Func::MAT4x4::LookAtLH(eyeWS, sphereCenterWS, up);

		// 4. Texel Snapping
		// Project the sphere center into Light Space
		Math::Vec3f centerLS = lightView.TransformPoint(sphereCenterWS);
		
		// Calculate texel size in world units
		// The orthographic width/height will be (radius * 2)
		float worldUnitsPerTexel = (sphereRadius * 2.0f) / config.shadowMapSize;

		// Snap the center to the nearest texel
		centerLS.x = std::floor(centerLS.x / worldUnitsPerTexel) * worldUnitsPerTexel;
		centerLS.y = std::floor(centerLS.y / worldUnitsPerTexel) * worldUnitsPerTexel;

		// Re-calculate view with snapped center
		Math::Mat4x4 invLightView = lightView.Inverse();
		Math::Vec3f snappedCenterWS = invLightView.TransformPoint(centerLS); 
		
		eyeWS = snappedCenterWS - lightDir * dist;
		lightView = Math::Func::MAT4x4::LookAtLH(eyeWS, snappedCenterWS, up);

		// 5. Build Orthographic Projection
		// Center is now (0,0) in Light Space (roughly).
		// Size is sphereRadius * 2.
		float r = sphereRadius;
		
		// Ensure enough Z range
		// centerLS.z is the depth of the center.
		// We pulled back by 'dist', so center should be at z = dist in Light Space? 
		// LookAtLH: z-axis is forward. eye is origin. target is +z.
		// dist is positive. eye = center - dir * dist.
		// So center is at eye + dir * dist. Which is +z * dist.
		
		float farZ_LS = dist + r + config.lightFarZ; 
		float nearZ_LS = dist - r - config.lightNearZ; 
		if (nearZ_LS < 0.1f) nearZ_LS = 0.1f;

		// Ortho box: [-r, +r] centered at (0,0) (because we looked at center)
		Math::Mat4x4 lightProj = Math::Func::MAT4x4::OrthographicMatrix(
			-r, r, r, -r, nearZ_LS, farZ_LS
		);

		outCtx.cascades[index].view = lightView;
		outCtx.cascades[index].proj = lightProj;
		outCtx.cascades[index].viewProj = lightView * lightProj;
	}

}
}
}

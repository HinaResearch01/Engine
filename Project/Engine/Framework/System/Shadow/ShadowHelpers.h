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

			// Calculates a bounding sphere that encloses the given frustum (defined by corners)
			// Returns center and radius.
			static void CalculateBoundingSphere(
				const Math::Vec3f corners[8],
				Math::Vec3f& outCenter,
				float& outRadius
			);
		};

		struct ShadowCascadeConfig {
			float shadowMapSize = 2048.0f;
			uint32_t cascadeCount = 4;
			float lambda = 0.96f; // Split lambda
			
			// Light parameters for bounds padding
			float lightNearZ = 0.1f;
			float lightFarZ = 1000.0f;
			float lightOrthoHalfSize = 100.0f;
		};

		class CSMGenerator {
		public:
			void Update(
				const CameraContext& camera,
				const Math::Vec3f& lightDir,
				const ShadowCascadeConfig& config,
				ShadowContext& outCtx
			);

		private:
			void CalcSplits(float nearZ, float farZ, float lambda);
			void BuildCascade(
				uint32_t index,
				const CameraContext& camera,
				const Math::Vec3f& lightDir,
				const ShadowCascadeConfig& config,
				ShadowContext& outCtx
			);

		private:
			std::array<float, 5> splits_{};
		};

	}

}
}

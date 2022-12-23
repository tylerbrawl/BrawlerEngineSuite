#pragma once
#include "MathConstants.hlsli"

namespace Util
{
	namespace Sampling
	{
		struct GGXImportanceSamplingParams
		{
			/// <summary>
			/// This is the *NORMALIZED* normal vector, in world space, of the relevant surface.
			/// </summary>
			float3 NormalWS;
			
			/// <summary>
			/// This is the GGX roughness value of the surface (a_g) squared.
			/// </summary>
			float GGXRoughnessSquared;
			
			/// <summary>
			/// Each component of this vector should contain a uniform randomized value 
			/// in the range [0.0f, 1.0f[.
			/// </summary>
			float2 NormalizedRandomValues;
		};
	}
}

// ---------------------------------------------------------------------------------------------------------------------

namespace Util
{
	namespace Sampling
	{
		float3 ImportanceSampleGGX(in const Util::Sampling::GGXImportanceSamplingParams params)
		{
			// The implementation for this was derived from the papers "Microfacet Models for
			// Refraction through Rough Surfaces" by Walter et al. and "Real Shading in Unreal
			// Engine 4" by Brian Karis.
			
			// First, get the spherical coordinates for the microfacet normal which we will
			// be returning. As described in "Microfacet Models for Refraction through Rough
			// Surfaces," these are as follows:
			//
			// Rho (p) = 1 (We want our microfacet normal m to be normalized.)
			// Theta = arctan((GGXRoughness * sqrt(NormalizedRandomValues.x)) / sqrt(1 - NormalizedRandomValues.x))
			// Phi = (TWO_PI * NormalizedRandomValues.y)
			//
			// These values define the microfacet normal m in spherical coordinates within
			// a tangent space we create. However, we don't use all of these values directly.
			
			const float phi = (BrawlerHLSL::TWO_PI * params.NormalizedRandomValues.y);
			
			// It isn't necessarily clear in "Real Shading in Unreal Engine 4" as to how their
			// equation was derived for their "CosTheta" variable in the provided implementation
			// for ImportanceSampleGGX(). These trigonometric properties lead to a proof:
			//
			// arctan(x) = y <-> x = tan(y)
			// cos(x) = +/- (1 / sqrt(1 + tan(x)^2))
			//
			// (Note, however, that cos(x) in the second property listed above will never be <= 0
			// for CosTheta. The proof is that the NDF D(m) = 0 for all microfacet normals m such
			// that dot(m, n) <= 0, and CosTheta = dot(m, n). This is explained in "Microfacet
			// Models for Refraction through Rough Surfaces.")
			//
			// If one were to replace tan(x) with the arctan value used to define Theta above,
			// they would arrive at the same equation as was derived by Karis.
			
			const float cosTheta = sqrt((1.0f - params.NormalizedRandomValues.x) / (1.0f + params.NormalizedRandomValues.x * (params.GGXRoughnessSquared - 1.0f)));
			const float sinTheta = sqrt(1.0f - (cosTheta * cosTheta));
			
			// Next, convert the microfacet normal to cartesian coordinates.
			const float3 m = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
			
			// We need to now transform the microfacet normal m from tangent space to world
			// space. This is a change-of-basis operation, and can be implemented using an
			// orthogonal matrix multiplication.
			
			// For the sake of importance sampling, it doesn't matter what the world space
			// coordinates of our tangent vector is, so long as the vector is orthogonal to
			// the relevant surface normal and pointing in the expected direction in tangent
			// space. So, to avoid singularities, we choose between either the +X-axis or the 
			// +Z-axis to cross with the surface normal.
			//
			// Oh, I know what your next question is: "Why those axes?" Using the left-hand
			// rule, we get a tangent vector pointing in the "right" direction by doing
			// either cross(n, +X) or cross(n, +Z), but not with cross (n, +Y). As we mentioned 
			// earlier, so long as the resulting vector is orthogonal to the surface normal and
			// points in the expected direction, we can use any vector to cross with the normal.
			// We only choose +X and +Z since we don't need to do a branch to determine which
			// side of the cross product n needs to be on, since cross(n, +X) and cross(n, +Z)
			// both produce a vector meeting our requirements.
			
			const float3 tempVector = (abs(params.NormalWS.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f));
			
			const float3 tangent = cross(params.NormalWS, tempVector);
			const float3 bitangent = cross(params.NormalWS, tangent);
			
			const float3x3 tangentToWorldMatrix = float3x3(tangent, bitangent, params.NormalWS);
			return mul(m, tangentToWorldMatrix);
		}
	}
}
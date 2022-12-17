module;
#include <vector>
#include <memory>
#include <span>
#include <cassert>
#include <cmath>
#include <assimp/scene.h>
#include <assimp/light.h>

module Brawler.AssimpSceneLoader;
import Brawler.Math.MathTypes;
import Brawler.PointLight;
import Brawler.SpotLight;

namespace
{
	Brawler::Math::Float4x4 CollapseWorldMatrixForAssimpNode(const aiNode& node)
	{
		// Assimp uses matrices designed for column vectors, so we need to multiply
		// their world matrices in the opposite order compared to the rest of the Brawler
		// Engine. Consider the following SceneGraph:
		//
		// Root -> A -> B -> C
		//
		// Let M_X be the world matrix for the SceneNode named X. Traditionally, we would
		// do the following to transform a point/vector v in homogeneous coordinates:
		//
		// v' = v * M_Root * M_A * M_B * M_C
		//
		// We want to find the matrix W defined as follows:
		//
		// W = M_Root * M_A * M_B * M_C
		// W = transpose(transpose(M_Root * M_A * M_B * M_C))
		// W = transpose(transpose(M_C) * transpose(M_B) * transpose(M_A) * transpose(M_Root))
		//
		// So, to get a world matrix which we can use in the rest of the Brawler Engine,
		// we can multiply the Assimp node transformation matrices in "reverse" order (that
		// is, reversed with respect to row vectors) and then take the transpose of that
		// result. This means starting from node and doing left-to-right matrix multiplications
		// with parent node transformation matrices.
		aiMatrix4x4 columnVectorWorldMatrix{ node.mTransformation };
		const aiNode* currParentNodePtr = node.mParent;

		while (currParentNodePtr != nullptr)
		{
			columnVectorWorldMatrix *= currParentNodePtr->mTransformation;
			currParentNodePtr = currParentNodePtr->mParent;
		}

		// Return the transpose of columnVectorWorldMatrix.
		return Brawler::Math::Float4x4{
			columnVectorWorldMatrix[0][0], columnVectorWorldMatrix[1][0], columnVectorWorldMatrix[2][0], columnVectorWorldMatrix[3][0],
			columnVectorWorldMatrix[0][1], columnVectorWorldMatrix[1][1], columnVectorWorldMatrix[2][1], columnVectorWorldMatrix[3][1],
			columnVectorWorldMatrix[0][2], columnVectorWorldMatrix[1][2], columnVectorWorldMatrix[2][2], columnVectorWorldMatrix[3][2],
			columnVectorWorldMatrix[0][3], columnVectorWorldMatrix[1][3], columnVectorWorldMatrix[2][3], columnVectorWorldMatrix[3][3]
		};
	}

	struct LightSceneNodeCreationParams
	{
		const aiScene& AssimpScene;
		const aiLight& AssimpLight;
	};

	std::unique_ptr<Brawler::PointLight> CreatePointLightSceneNode(const LightSceneNodeCreationParams& params)
	{
		assert(params.AssimpLight.mType == aiLightSourceType::aiLightSource_POINT && "ERROR: An attempt was made to construct a PointLight SceneNode instance from an aiLight instance, but said aiLight was not actually a point light!");
		
		std::unique_ptr<Brawler::PointLight> pointLightSceneNodePtr{ std::make_unique<Brawler::PointLight>() };

		// Assimp does not seem to be designed for PBR, as the aiLight struct contains
		// numerous fields which aren't physically based. We'll need to make some assumptions.

		// Assimp doesn't specify a luminous intensity value, so we assume that the specified
		// light color is already scaled by luminous intensity. In that case, we infer the
		// luminous intensity value used to scale the light color by getting its magnitude. We
		// then use this magnitude as the luminous intensity value and use the normalized light
		// color vector as the light color.
		{
			const aiColor3D aiLightColor{ params.AssimpLight.mColorDiffuse };
			Brawler::Math::Float3 lightColor{ aiLightColor.r, aiLightColor.g, aiLightColor.b };

			const float luminousIntensityInCandelas = lightColor.GetLength();
			pointLightSceneNodePtr->SetLuminousIntensity(luminousIntensityInCandelas);

			lightColor /= std::max<float>(luminousIntensityInCandelas, 0.00001f);
			pointLightSceneNodePtr->SetLightColor(lightColor);
		}

		// The physically-correct way to do light attenuation is via the inverse-square
		// attenuation law, which states that luminous intensity is inversely proportional
		// to the squared distance between the light and the surface being shaded.
		//
		// This is not what Assimp does. Instead, Assimp provides a set of polynomial
		// coefficients which allows one to solve for the attenuation of the light source
		// as follows:
		//
		// AssimpLightAttenuation = 1.0f / (A * d^2 + B * d + C)
		//
		// where A = aiLight::mAttenuationQuadratic, B = aiLight::mAttenuationLinear,
		// C = aiLight::mAttenuationConstant, and d is the distance from the light to the
		// surface being shaded.
		//
		// Notice, however, that if A = 1 and B = C = 0, then AssimpLightAttenuation directly
		// corresponds to the inverse-square attenuation law. Even if this follows the inverse-
		// square attenuation law, there is another problem: The value of AssimpLightAttenuation 
		// never reaches zero, implying that every point in the world will be affected by the light.
		//
		// While this is technically physically correct, it is not efficient. In real-time 
		// rendering, we typically use a windowing function to ensure that the light's contribution 
		// ends at some point. This windowing function relies on a light distance parameter which 
		// Assimp does not provide. We can, however, try to find a distance value by solving the 
		// polynomial equation.
		//
		// Since AssimpLightAttenuation never reaches zero, we'll need to use a very small
		// value to solve for (say, 0.01).
		static constexpr float LIGHT_ATTENUATION_THRESHOLD = 0.01f;

		// Now, we solve for d:
		//
		// 1.0 / (A * d^2 + B * d + C) = 0.01
		// 0.01 * (A * d^2 + B * d + C) = 1.0
		// 0.01A * d^2 + 0.01B * d + 0.01C - 1.0 = 0
		//
		// d = (-b +/- sqrt(b^2 - 4ac)) / 2a
		//
		// where:
		//   a = 0.01A
		//   b = 0.01B
		//   c = 0.01C - 1.0
		//
		// If A = 0, then the equation for d is undefined, but we can then solve for a linear
		// equation:
		//
		// 1.0 / (B * d + C) = 0.01
		// 0.01 * (B * d + C) = 1.0
		// 0.01B * d + 0.01C = 1.0
		// d = (1.0 - 0.01C) / 0.01B
		//
		// If B = 0, then this equation for d is also undefined, and we have a pretty major problem.
		// In that case, we'll just use the default maximum light distance by not calling
		// PointLight::SetMaximumLightDistance().
		{
			const float A = params.AssimpLight.mAttenuationQuadratic;
			const float B = params.AssimpLight.mAttenuationLinear;
			const float C = params.AssimpLight.mAttenuationConstant;

			if (A > 0)
			{
				// Technically, there are two solutions to d in this case, but we only need one.
				const float a = (LIGHT_ATTENUATION_THRESHOLD * A);
				const float b = (LIGHT_ATTENUATION_THRESHOLD * B);
				const float c = ((LIGHT_ATTENUATION_THRESHOLD * C) - 1.0f);

				const float bSquared = (b * b);
				const float four_a_c = (4.0f * a * c);
				const float two_a = (2.0f * a);

				const float maxLightDistanceInMeters = ((-b + std::sqrtf(bSquared - four_a_c)) / two_a);
				pointLightSceneNodePtr->SetMaximumLightDistance(std::max<float>(maxLightDistanceInMeters, 0.01f));
			}
			else if (B > 0)
			{
				const float maxLightDistanceInMeters = (1.0f - (LIGHT_ATTENUATION_THRESHOLD * C)) / (LIGHT_ATTENUATION_THRESHOLD * B);
				pointLightSceneNodePtr->SetMaximumLightDistance(std::max<float>(maxLightDistanceInMeters, 0.01f));
			}
		}

		return pointLightSceneNodePtr;
	}
}

namespace Brawler
{
	void AssimpSceneLoader::LoadScene(const aiScene& scene)
	{

	}

	SceneGraph AssimpSceneLoader::ExtractSceneGraph()
	{
		return std::move(mSceneGraph);
	}

	std::vector<std::unique_ptr<SceneNode>> AssimpSceneLoader::CreateLightSceneNodes(const aiScene& scene)
	{
		const std::span<const aiLight*> lightPtrSpan{ scene.mLights, scene.mNumLights };

		std::vector<std::unique_ptr<SceneNode>> lightSceneNodePtrArr{};
		lightSceneNodePtrArr.reserve(lightPtrSpan.size());

		for (const auto lightPtr : lightPtrSpan)
		{

		}
	}
}
module;
#include <memory>
#include <vector>
#include <span>
#include <cassert>
#include <assimp/scene.h>
#include <assimp/light.h>

module Brawler.AssimpSceneLoader;
import :AssimpLoadingUtil;
import Brawler.Math.MathTypes;
import Brawler.PointLight;
import Brawler.SpotLight;
import Brawler.TransformComponent;

namespace
{
	template <typename SceneNodeType>
	void UpdatePunctualLightParameters(SceneNodeType& lightSceneNode, const aiLight& assimpLight)
	{
		// Assimp does not seem to be designed for PBR, as the aiLight struct contains
		// numerous fields which aren't physically based. We'll need to make some assumptions.

		// Assimp doesn't specify a luminous intensity value, so we assume that the specified
		// light color is already scaled by luminous intensity. In that case, we infer the
		// luminous intensity value used to scale the light color by getting its magnitude. We
		// then use this magnitude as the luminous intensity value and use the normalized light
		// color vector as the light color.
		{
			const aiColor3D aiLightColor{ assimpLight.mColorDiffuse };
			Brawler::Math::Float3 lightColor{ aiLightColor.r, aiLightColor.g, aiLightColor.b };

			const float luminousIntensityInCandelas = lightColor.GetLength();
			lightSceneNode.SetLuminousIntensity(luminousIntensityInCandelas);

			lightColor /= std::max<float>(luminousIntensityInCandelas, 0.00001f);
			lightSceneNode.SetLightColor(lightColor);
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
			const float A = assimpLight.mAttenuationQuadratic;
			const float B = assimpLight.mAttenuationLinear;
			const float C = assimpLight.mAttenuationConstant;

			if (A > 0)
			{
				const float a = (LIGHT_ATTENUATION_THRESHOLD * A);
				const float b = (LIGHT_ATTENUATION_THRESHOLD * B);
				const float c = ((LIGHT_ATTENUATION_THRESHOLD * C) - 1.0f);

				const float bSquared = (b * b);
				const float four_a_c = (4.0f * a * c);
				const float two_a = (2.0f * a);

				// Technically, there are two solutions to d in this case, but we only need one.
				const float maxLightDistanceInMeters = ((-b + std::sqrtf(bSquared - four_a_c)) / two_a);
				lightSceneNode.SetMaximumLightDistance(std::max<float>(maxLightDistanceInMeters, 0.01f));
			}
			else if (B > 0)
			{
				const float maxLightDistanceInMeters = (1.0f - (LIGHT_ATTENUATION_THRESHOLD * C)) / (LIGHT_ATTENUATION_THRESHOLD * B);
				lightSceneNode.SetMaximumLightDistance(std::max<float>(maxLightDistanceInMeters, 0.01f));
			}
		}
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

		{
			const aiNode* const transformationNodePtr = params.AssimpScene.mRootNode->FindNode(params.AssimpLight.mName);
			assert(transformationNodePtr != nullptr && "ERROR: Assimp could not find the transformation node for a point light! (This should never happen, since Assimp's documentation states that this node must be present.)");

			// For point lights, we only need to worry about translation.
			const Brawler::Math::Float4x4 worldMatrix{ Util::AssimpLoading::CollapseWorldMatrixForAssimpNode(*transformationNodePtr) };

			Brawler::TransformComponent* const transformComponentPtr = pointLightSceneNodePtr->GetComponent<Brawler::TransformComponent>();
			assert(transformComponentPtr != nullptr);

			Brawler::Math::Float3 translationVector{ worldMatrix.GetElement(3, 0), worldMatrix.GetElement(3, 1), worldMatrix.GetElement(3, 2) };
			transformComponentPtr->SetTranslation(std::move(translationVector));
		}

		UpdatePunctualLightParameters(*pointLightSceneNodePtr, params.AssimpLight);

		return pointLightSceneNodePtr;
	}

	std::unique_ptr<Brawler::SpotLight> CreateSpotLightSceneNode(const LightSceneNodeCreationParams& params)
	{
		assert(params.AssimpLight.mType == aiLightSourceType::aiLightSource_SPOT && "ERROR: An attempt was made to construct a SpotLight SceneNode instance from an aiLight instance, but said aiLight was not actually a spotlight!");

		std::unique_ptr<Brawler::SpotLight> spotLightSceneNodePtr{ std::make_unique<Brawler::SpotLight>() };

		{
			const aiNode* const transformationNodePtr = params.AssimpScene.mRootNode->FindNode(params.AssimpLight.mName);
			assert(transformationNodePtr != nullptr && "ERROR: Assimp could not find the transformation node for a spotlight! (This should never happen, since Assimp's documentation states that this node must be present.)");

			Util::AssimpLoading::TransformSceneNode(*spotLightSceneNodePtr, *transformationNodePtr);
		}

		UpdatePunctualLightParameters(*spotLightSceneNodePtr, params.AssimpLight);

		spotLightSceneNodePtr->SetUmbraAngle(params.AssimpLight.mAngleOuterCone);
		spotLightSceneNodePtr->SetPenumbraAngle(params.AssimpLight.mAngleInnerCone);

		return spotLightSceneNodePtr;
	}
}

namespace Brawler
{
	void AssimpLightLoader::LoadLights(const aiScene& scene)
	{
		mLightSceneNodePtrArr.clear();

		const std::span<const aiLight*> lightPtrSpan{ scene.mLights, scene.mNumLights };
		mLightSceneNodePtrArr.reserve(lightPtrSpan.size());

		for (const auto lightPtr : lightPtrSpan)
		{
			assert(lightPtr != nullptr);

			const LightSceneNodeCreationParams sceneNodeCreationParams{
				.AssimpScene{ scene },
				.AssimpLight{ *lightPtr }
			};

			switch (lightPtr->mType)
			{
			case aiLightSourceType::aiLightSource_POINT:
			{
				mLightSceneNodePtrArr.push_back(CreatePointLightSceneNode(sceneNodeCreationParams));
				break;
			}

			case aiLightSourceType::aiLightSource_SPOT:
			{
				mLightSceneNodePtrArr.push_back(CreateSpotLightSceneNode(sceneNodeCreationParams));
				break;
			}

			case aiLightSourceType::aiLightSource_DIRECTIONAL:
			{
				assert(false && "TODO: Add support for directional lights!");
				break;
			}

			default: [[unlikely]]
			{
				assert(false && "ERROR: An unrecognized light type was detected when loading an Assimp scene!");
				break;
			}
			}
		}
	}

	std::span<std::unique_ptr<SceneNode>> AssimpLightLoader::GetLightSceneNodeSpan()
	{
		return std::span<std::unique_ptr<SceneNode>>{ mLightSceneNodePtrArr };
	}

	std::span<const std::unique_ptr<SceneNode>> AssimpLightLoader::GetLightSceneNodeSpan() const
	{
		return std::span<const std::unique_ptr<SceneNode>>{ mLightSceneNodePtrArr };
	}
}
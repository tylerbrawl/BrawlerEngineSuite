module;
#include <cassert>
#include <DirectXMath/DirectXMath.h>

module Brawler.SpotLight;
import Brawler.Math.MathConstants;
import Util.Math;
import Brawler.GPUSceneManager;
import Brawler.D3D12.BufferResource;
import Brawler.GPUSceneBufferID;
import Brawler.TransformComponent;

namespace
{
	constexpr float ConvertLuminousPowerToLuminousIntensity(const float luminousPowerInLumens)
	{
		// From "Moving Frostbite to Physically Based Rendering 3.0:"
		//
		// LuminousIntensity = LuminousPower lumens / PI steradians = (LuminousPower / PI) lm/sr = (LuminousPower / PI) cd
		
		return (luminousPowerInLumens / Brawler::Math::PI);
	}

	// According to https://www.blancocountynightsky.org/basics.php, "[a] standard 100-watt incandescent light bulb
	// produces about 1600 lumens." 
	static constexpr float DEFAULT_LUMINOUS_INTENSITY = ConvertLuminousPowerToLuminousIntensity(1600.0f);

	static constexpr Math::Float3 DEFAULT_LIGHT_COLOR{ 1.0f, 1.0f, 1.0f };

	// 5 meters is completely empirical based on real-world lightbulbs, and probably isn't a good fit for real-time
	// rendering.
	static constexpr float DEFAULT_MAXIMUM_DISTANCE = 5.0f;

	// By default, all spotlights point towards the +Z axis.
	static constexpr Brawler::Math::Float3 DEFAULT_LIGHT_DIRECTION{ Brawler::Math::Z_AXIS };
	static_assert(DEFAULT_LIGHT_DIRECTION.IsNormalized());

	static constexpr float DEFAULT_UMBRA_ANGLE = Util::Math::DegreesToRadians(60.0f);
	static constexpr float DEFAULT_PENUMBRA_ANGLE = Util::Math::DegreesToRadians(30.0f);

	static constexpr Brawler::GPUSceneTypes::SpotLight DEFAULT_SPOTLIGHT_PARAMS{
		.LuminousIntensity = DEFAULT_LUMINOUS_INTENSITY,
		.LightColor{ DEFAULT_LIGHT_COLOR },
		.InverseMaxDistanceSquared = (1.0f / (DEFAULT_MAX_DISTANCE * DEFAULT_MAX_DISTANCE)),

		// We assume that the default rotation for the SceneNode is the identity quaternion, i.e., that no rotation
		// occurs. In that case, the world space direction of the light is the same as its direction in its
		// local coordinate system.
		.DirectionWS{ DEFAULT_LIGHT_DIRECTION },

		.CosineUmbraAngle = Util::Math::GetCosineAngle(DEFAULT_UMBRA_ANGLE),
		.CosinePenumbraAngle = Util::Math::GetCosineAngle(DEFAULT_PENUMBRA_ANGLE)
	};
}

namespace Brawler
{
	SpotLight::SpotLight() :
		SceneNode(),
		mSpotLightBufferSubAllocation(),
		mLightDescriptorUpdater(),
		mSpotLightData(DEFAULT_SPOTLIGHT_PARAMS),
		mLightDirectionLS(DEFAULT_LIGHT_DIRECTION),
		mIsGPUSceneDataDirty(true)
	{
		[[maybe_unused]] const bool wasReservationSuccessful = GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::SPOTLIGHT_BUFFER>().AssignReservation(mSpotLightBufferSubAllocation);
		assert(wasReservationSuccessful && "ERROR: An attempt to create a reservation from the SpotLight GPU scene buffer failed!");

		// Add all required I_Component instances to this SpotLight instance.
		CreateComponent<TransformComponent>();

		// Initialize the LightDescriptor GPU scene buffer data.
		mLightDescriptor.InitializeGPUSceneBufferData(GetSpotLightBufferIndex());
	}

	void SpotLight::SetLuminousIntensity(const float intensityInCandelas)
	{
		assert(intensityInCandelas >= 0.0f && "ERROR: An attempt was made to specify negative luminous intensity value in a call to SpotLight::SetLuminousIntensity()!");

		mSpotLightData.LuminousIntensity = intensityInCandelas;
		MarkGPUSceneDataAsDirty();
	}

	void SpotLight::SetLightColor(const Math::Float3 lightColor)
	{
		assert(lightColor.GetX() >= 0.0f && "ERROR: An attempt was made to specify a light color with a negative R channel in a call to SpotLight::SetLightColor()!");
		assert(lightColor.GetY() >= 0.0f && "ERROR: An attempt was made to specify a light color with a negative G channel in a call to SpotLight::SetLightColor()!");
		assert(lightColor.GetZ() >= 0.0f && "ERROR: An attempt was made to specify a light color with a negative B channel in a call to SpotLight::SetLightColor()!");

		mSpotLightData.LightColor = DirectX::XMFLOAT3{ lightColor.GetX(), lightColor.GetY(), lightColor.GetZ() };
		MarkGPUSceneDataAsDirty();
	}

	void SpotLight::SetMaximumLightDistance(const float maxDistanceInMeters)
	{
		assert(maxDistanceInMeters >= 0.01f && "ERROR: An attempt was made to specify a maximum light distance which was less than the size of a punctual light (1 cm) in a call to SpotLight::SetMaximumLightDistance()!");

		const float inverseDistance = (1.0f / maxDistanceInMeters);
		mSpotLightData.InverseMaxDistanceSquared = (inverseDistance * inverseDistance);

		MarkGPUSceneDataAsDirty();
	}

	void SpotLight::SetLightDirection(const Math::Float3 lightDirectionLS)
	{
		assert(lightDirectionLS.IsNormalized() && "ERROR: An attempt was made to specify an unnormalized light direction in a call to SpotLight::SetLightDirection()!");

		mLightDirectionLS = lightDirectionLS;
		MarkGPUSceneDataAsDirty();
	}

	bool SpotLight::IsGPUSceneDataDirty() const
	{
		return mIsGPUSceneDataDirty;
	}

	void SpotLight::MarkGPUSceneDataAsDirty()
	{
		mIsGPUSceneDataDirty = true;
	}

	void SpotLight::CheckForRotationChange(const TransformComponent& transformComponent)
	{
		const Math::Float3 currFrameLightDirectionWS{ transformComponent.GetRotation().RotateVector(mLightDirectionLS) };
		const Math::Float3& prevFrameLightDirectionWS{ mSpotLightData.DirectionWS };

		if (currFrameLightDirectionWS != prevFrameLightDirectionWS) [[unlikely]]
		{
			mSpotLightInfo.DirectionWS = DirectX::XMFLOAT3{ currFrameLightDirectionWS.GetX(), currFrameLightDirectionWS.GetY(), currFrameLightDirectionWS.GetZ() };
			MarkGPUSceneDataAsDirty();
		}
	}

	void SpotLight::CheckForTranslationChange(const TransformComponent& transformComponent)
	{
		const Math::Float3& currFramePositionWS{ transformComponent.GetTranslation() };
		const Math::Float3& prevFramePositionWS{ mSpotLightData.PositionWS };

		if (currFramePositionWS != prevFramePositionWS) [[unlikely]]
		{
			mSpotLightInfo.PositionWS = DirectX::XMFLOAT3{ currFramePositionWS.GetX(), currFramePositionWS.GetY(), currFramePositionWS.GetZ() };
			MarkGPUSceneDataAsDirty();
		}
	}

	std::uint32_t SpotLight::GetSpotLightBufferIndex() const
	{
		return (mSpotLightBufferSubAllocation.GetOffsetFromBufferStart() / sizeof(GPUSceneTypes::SpotLight));
	}
}
module;
#include <cassert>
#include <cmath>
#include <span>
#include <DirectXMath/DirectXMath.h>

module Brawler.SpotLight;
import Brawler.Math.MathConstants;
import Util.Math;
import Brawler.GPUSceneManager;
import Brawler.D3D12.BufferResource;
import Brawler.GPUSceneBufferID;
import Brawler.TransformComponent;
import Brawler.GPUSceneBufferUpdateOperation;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdateRenderModule;

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
}

namespace Brawler
{
	SpotLight::SpotLight() :
		SceneNode(),
		mSpotLightBufferSubAllocation(),
		mLightDescriptorUpdater(),
		mSpotLightInfo(SpotLightInfo{
			.LightColor{ DEFAULT_LIGHT_COLOR },
			.LuminousIntensityInCandelas = DEFAULT_LUMINOUS_INTENSITY,
			.MaxDistanceInMeters = DEFAULT_MAXIMUM_DISTANCE,
			.UmbraAngleInRadians = DEFAULT_UMBRA_ANGLE,
			.PenumbraAngleInRadians = DEFAULT_PENUMBRA_ANGLE
		}),
		mCachedDirectionWS(),
		mCachedTranslation(),
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

	void SpotLight::LateUpdate(const float dt)
	{
		const TransformComponent* const transformComponentPtr = GetComponent<const TransformComponent>();
		assert(transformComponentPtr != nullptr);

		CheckForRotationChange(*transformComponentPtr);
		CheckForTranslationChange(*transformComponentPtr);

		if (IsGPUSceneDataDirty()) [[unlikely]]
			UpdateGPUSceneBufferData();
	}

	void SpotLight::SetLuminousIntensity(const float intensityInCandelas)
	{
		assert(intensityInCandelas >= 0.0f && "ERROR: An attempt was made to specify a negative luminous intensity value in a call to SpotLight::SetLuminousIntensity()!");

		mSpotLightInfo.LuminousIntensityInCandelas = intensityInCandelas;
		MarkGPUSceneDataAsDirty();
	}

	void SpotLight::SetLightColor(const Math::Float3 lightColor)
	{
		assert(lightColor.GetX() >= 0.0f && "ERROR: An attempt was made to specify a light color with a negative R channel in a call to SpotLight::SetLightColor()!");
		assert(lightColor.GetY() >= 0.0f && "ERROR: An attempt was made to specify a light color with a negative G channel in a call to SpotLight::SetLightColor()!");
		assert(lightColor.GetZ() >= 0.0f && "ERROR: An attempt was made to specify a light color with a negative B channel in a call to SpotLight::SetLightColor()!");

		mSpotLightInfo.LightColor = lightColor;
		MarkGPUSceneDataAsDirty();
	}

	void SpotLight::SetMaximumLightDistance(const float maxDistanceInMeters)
	{
		assert(maxDistanceInMeters >= 0.01f && "ERROR: An attempt was made to specify a maximum light distance which was less than the size of a punctual light (1 cm) in a call to SpotLight::SetMaximumLightDistance()!");

		mSpotLightInfo.MaxDistanceInMeters = maxDistanceInMeters;
		MarkGPUSceneDataAsDirty();
	}

	void SpotLight::SetLightDirection(const Math::Float3 lightDirectionLS)
	{
		assert(lightDirectionLS.IsNormalized() && "ERROR: An attempt was made to specify an unnormalized light direction in a call to SpotLight::SetLightDirection()!");

		mLightDirectionLS = lightDirectionLS;

		// Don't bother marking the GPU scene buffer data as dirty; if SpotLight::CheckForRotationChange()
		// notices that the light direction in world space has changed since the previous frame, then it
		// will do that for us.
	}

	void SpotLight::SetUmbraAngle(const float umbraAngle)
	{
		mSpotLightInfo.UmbraAngleInRadians = umbraAngle;
		MarkGPUSceneDataAsDirty();
	}

	void SpotLight::SetPenumbraAngle(const float penumbraAngle)
	{
		mSpotLightInfo.PenumbraAngleInRadians = penumbraAngle;
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

		if (currFrameLightDirectionWS != mCachedDirectionWS) [[unlikely]]
		{
			mCachedDirectionWS = currFrameLightDirectionWS;
			MarkGPUSceneDataAsDirty();
		}
	}

	void SpotLight::CheckForTranslationChange(const TransformComponent& transformComponent)
	{
		const Math::Float3& currFramePositionWS{ transformComponent.GetTranslation() };

		if (currFramePositionWS != mCachedTranslation) [[unlikely]]
		{
			mCachedTranslation = currFramePositionWS;
			MarkGPUSceneDataAsDirty();
		}
	}

	void SpotLight::UpdateGPUSceneBufferData()
	{
		assert(IsGPUSceneDataDirty() && "ERROR: An attempt was made to call SpotLight::UpdateGPUSceneBufferData() when the GPU scene buffer data didn't need to be updated!");

		// Ensure that the specified parameters are valid.
		assert(mSpotLightInfo.PenumbraAngleInRadians <= mSpotLightInfo.UmbraAngleInRadians && "ERROR: The penumbra angle of a SpotLight instance cannot be larger than its umbra angle!");

		const Math::Float3 scaledLightColor{ mSpotLightInfo.LightColor * mSpotLightInfo.LuminousIntensityInCandelas };
		const GPUSceneTypes::SpotLight gpuSceneBufferData{
			.PositionWS{ mCachedTranslation.GetX(), mCachedTranslation.GetY(), mCachedTranslation.GetZ() },
			.InverseMaxDistanceSquared = (1.0f / (mSpotLightInfo.MaxDistanceInMeters * mSpotLightInfo.MaxDistanceInMeters)),
			.ScaledLightColor{ scaledLightColor.GetX(), scaledLightColor.GetY(), scaledLightColor.GetZ() },
			.CosineUmbraAngle = std::cosf(mSpotLightInfo.UmbraAngleInRadians),
			.DirectionWS{ mCachedDirectionWS.GetX(), mCachedDirectionWS.GetY(), mCachedDirectionWS.GetZ() },
			.CosinePenumbraAngle = std::cosf(mSpotLightInfo.PenumbraAngleInRadians)
		};

		GPUSceneBufferUpdateOperation<GPUSceneBufferID::SPOTLIGHT_BUFFER> spotLightUpdateOperation{ mSpotLightBufferSubAllocation.GetBufferCopyRegion() };
		spotLightUpdateOperation.SetUpdateSourceData(std::span<const GPUSceneTypes::SpotLight>{ &gpuSceneBufferData, 1 });

		Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(spotLightUpdateOperation));

		mIsGPUSceneDataDirty = false;
	}

	std::uint32_t SpotLight::GetSpotLightBufferIndex() const
	{
		return (mSpotLightBufferSubAllocation.GetOffsetFromBufferStart() / sizeof(GPUSceneTypes::SpotLight));
	}
}
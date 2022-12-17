module;
#include <cassert>
#include <DirectXMath/DirectXMath.h>

module Brawler.PointLight;
import Brawler.Math.MathConstants;
import Brawler.GPUSceneManager;
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
		// LuminousIntensity = LuminousPower lumens / (4 * PI steradians) = (LuminousPower / (4 * PI)) lm/sr = (LuminousPower / (4 * PI)) cd

		constexpr float FOUR_PI = (4.0f * Brawler::Math::PI);
		constexpr float INVERSE_FOUR_PI = (1.0f / FOUR_PI);

		return (luminousPowerInLumens * INVERSE_FOUR_PI);
	}

	// According to https://www.blancocountynightsky.org/basics.php, "[a] standard 100-watt incandescent light bulb
	// produces about 1600 lumens." 
	static constexpr float DEFAULT_LUMINOUS_INTENSITY = ConvertLuminousPowerToLuminousIntensity(1600.0f);

	static constexpr DirectX::XMFLOAT3 DEFAULT_LIGHT_COLOR{ 1.0f, 1.0f, 1.0f };

	// 5 meters is completely empirical based on real-world lightbulbs, and probably isn't a good fit for real-time
	// rendering.
	static constexpr float DEFAULT_MAXIMUM_DISTANCE = 5.0f;
}

namespace Brawler 
{
	PointLight::PointLight() :
		SceneNode(),
		mPointLightBufferSubAllocation(),
		mLightDescriptorUpdater(),
		mPointLightInfo(PointLightInfo{
			.LightColor{ DEFAULT_LIGHT_COLOR },
			.LuminousIntensityInCandelas = DEFAULT_LUMINOUS_INTENSITY,
			.MaxDistanceInMeters = DEFAULT_MAX_DISTANCE
		}),
		mCachedTranslation(),
		mIsGPUSceneDataDirty(true)
	{
		[[maybe_unused]] const bool wasReservationSuccessful = GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::POINT_LIGHT_BUFFER>().AssignReservation(mPointLightBufferSubAllocation);
		assert(wasReservationSuccessful && "ERROR: An attempt to create a reservation from the PointLight GPU scene buffer failed!");

		// Add all required I_Component instances to this PointLight instance.
		CreateComponent<TransformComponent>();

		// Initialize the LightDescriptor GPU scene buffer data.
		mLightDescriptorUpdater.InitializeGPUSceneBufferData(GetPointLightBufferIndex());
	}

	void PointLight::LateUpdate(const float dt)
	{
		CheckForTranslationChange();

		if (IsGPUSceneDataDirty()) [[unlikely]]
			UpdateGPUSceneBufferData();
	}

	void PointLight::SetLuminousIntensity(const float intensityInCandelas)
	{
		assert(intensityInCandelas >= 0.0f && "ERROR: An attempt was made to specify a negative luminous intensity value in a call to PointLight::SetLuminousIntensity()!");
		
		mPointLightInfo.LuminousIntensityInCandelas = intensityInCandelas;
		MarkGPUSceneDataAsDirty();
	}

	void PointLight::SetLightColor(const Math::Float3 lightColor)
	{
		assert(lightColor.GetX() >= 0.0f && "ERROR: An attempt was made to specify a light color with a negative R channel in a call to PointLight::SetLightColor()!");
		assert(lightColor.GetY() >= 0.0f && "ERROR: An attempt was made to specify a light color with a negative G channel in a call to PointLight::SetLightColor()!");
		assert(lightColor.GetZ() >= 0.0f && "ERROR: An attempt was made to specify a light color with a negative B channel in a call to PointLight::SetLightColor()!");

		mPointLightInfo.LightColor = lightColor;
		MarkGPUSceneDataAsDirty();
	}

	void PointLight::SetMaximumLightDistance(const float maxDistanceInMeters)
	{
		assert(maxDistanceInMeters >= 0.01f && "ERROR: An attempt was made to specify a maximum light distance which was less than the size of a punctual light (1 cm) in a call to PointLight::SetMaximumLightDistance()!");

		mPointLightInfo.MaxDistanceInMeters = maxDistanceInMeters;
		MarkGPUSceneDataAsDirty();
	}

	bool PointLight::IsGPUSceneDataDirty() const
	{
		return mIsGPUSceneDataDirty;
	}

	void PointLight::MarkGPUSceneDataAsDirty()
	{
		mIsGPUSceneDataDirty = true;
	}

	void PointLight::CheckForTranslationChange()
	{
		const TransformComponent* const transformComponentPtr = GetComponent<const TransformComponent>();
		assert(transformComponentPtr != nullptr);

		const Math::Float3& currFrameTranslation{ transformComponentPtr->GetTranslation() };

		if (currFrameTranslation != mCachedTranslation) [[unlikely]]
		{
			mCachedTranslation = currFrameTranslation;
			MarkGPUSceneDataAsDirty();
		}
	}

	void PointLight::UpdateGPUSceneBufferData()
	{
		assert(IsGPUSceneDataDirty() && "ERROR: An attempt was made to call PointLight::UpdateGPUSceneBufferData() when the GPU scene buffer data didn't need to be updated!");

		const Math::Float3 scaledLightColor{ mPointLightInfo.LightColor * mPointLightInfo.LuminousIntensityInCandelas };
		const GPUSceneTypes::PointLight gpuSceneBufferData{
			.PositionWS{ mCachedTranslation.GetX(), mCachedTranslation.GetY(), mCachedTranslation.GetZ() },
			.InverseMaxDistanceSquared = (1.0f / (mPointLightInfo.MaxDistanceInMeters * mPointLightInfo.MaxDistanceInMeters)),
			.ScaledLightColor{ scaledLightColor.GetX(), scaledLightColor.GetY(), scaledLightColor.GetZ() }
		};

		GPUSceneBufferUpdateOperation<GPUSceneBufferID::POINT_LIGHT_BUFFER> pointLightUpdateOperation{ mPointLightBufferSubAllocation.GetBufferCopyRegion() };
		pointLightUpdateOperation.SetUpdateSourceData(std::span<const GPUSceneTypes::PointLight>{ &gpuSceneBufferData, 1 });

		Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(pointLightUpdateOperation));

		mIsGPUSceneDataDirty = false;
	}

	std::uint32_t PointLight::GetPointLightBufferIndex() const
	{
		return static_cast<std::uint32_t>(mPointLightBufferSubAllocation.GetOffsetFromBufferStart() / sizeof(GPUSceneTypes::PointLight));
	}
}
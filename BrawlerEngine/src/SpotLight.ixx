module;

export module Brawler.SpotLight;
import Brawler.SceneNode;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.LightDescriptorUpdater;
import Brawler.LightID;
import Brawler.Math.MathTypes;
import Brawler.TransformComponent;

export namespace Brawler 
{
	class SpotLight final : public SceneNode
	{
	private:
		struct SpotLightInfo
		{
			Math::Float3 LightColor;
			float LuminousIntensityInCandelas;
			float MaxDistanceInMeters;
			float UmbraAngleInRadians;
			float PenumbraAngleInRadians;
		};

	public:
		SpotLight();

		SpotLight(const SpotLight& rhs) = delete;
		SpotLight& operator=(const SpotLight& rhs) = delete;

		SpotLight(SpotLight&& rhs) noexcept = default;
		SpotLight& operator=(SpotLight&& rhs) noexcept = default;

		void LateUpdate(const float dt) override;

		/// <summary>
		/// Sets the luminous intensity, in Candelas (cd), of the SpotLight instance.
		/// 
		/// NOTE: Frostbite exposes luminous power (in lumens, or lm) to artists and converts
		/// these values to luminous intensity for spotlights as follows:
		///
		/// LuminousIntensity = LuminousPower lumens / PI steradians = (LuminousPower / PI) lm/sr = (LuminousPower / PI) cd
		/// 
		/// This is intentionally *NOT* mathematically correct; see the comments in
		/// SpotLight::CalculateAttenuatedLuminance() in SpotLight.hlsli for more details.
		/// </summary>
		/// <param name="intensityInCandelas">
		/// - The luminous intensity, in Candelas (cd), of the light.
		/// </param>
		void SetLuminousIntensity(const float intensityInCandelas);

		/// <summary>
		/// Sets the color of this SpotLight instance expressed as an RGB triple in (linear) sRGB 
		/// color space.
		/// 
		/// Future work would be to allow this value to be computed from, e.g., color temperature.
		/// </summary>
		/// <param name="lightColor">
		/// - The color of the light as an RGB triple in (linear) sRGB color space.
		/// </param>
		void SetLightColor(const Math::Float3 lightColor);

		/// <summary>
		/// Sets the maximum distance, in meters, which this SpotLight instance reaches. If
		/// one were to imagine the spotlight as a cone, then this value would be the magnitude
		/// of the *UNNORMALIZED* direction vector.
		/// 
		/// *NOTE*: The maximum distance of a SpotLight instance *MUST* be at least 1 cm, which is
		/// the "size" of a punctual light in the Brawler Engine. An assert is fired in Debug builds
		/// if this is not the case.
		/// </summary>
		/// <param name="maxDistanceInMeters">
		/// - The maximum distance, in meters, which this spotlight reaches.
		/// </param>
		void SetMaximumLightDistance(const float maxDistanceInMeters);

		/// <summary>
		/// Sets the direction of this SpotLight instance using a vector specified in the light's
		/// local coordinate system. (This implies that the actual direction of the light in
		/// world space will change based on the rotation specified in the TransformComponent of
		/// the SpotLight instance.)
		/// 
		/// By default, all spot lights point towards the +Z axis.
		/// 
		/// *NOTE*: directionLS *MUST* be normalized. An assert is fired in Debug builds if this
		/// is not the case.
		/// </summary>
		/// <param name="directionLS">
		/// - The normalized vector, in the light's local coordinate system, which specifies its
		///   direction.
		/// </param>
		void SetLightDirection(const Math::Float3 lightDirectionLS);

		/// <summary>
		/// Sets the umbra angle of this SpotLight instance. The umbra angle describes the angle
		/// from the light's direction vector to the maximum extent of the cone bounding volume
		/// of the spotlight.
		/// 
		/// Necessarily, PenumbraAngle <= UmbraAngle (and thus cos(PenumbraAngle) >= cos(UmbraAngle)).
		/// 
		/// This value is sometimes less formally referred to as the "outer angle" of the spotlight.
		/// </summary>
		/// <param name="umbraAngle">
		/// - The umbra/outer angle, in radians, of the spotlight.
		/// </param>
		void SetUmbraAngle(const float umbraAngle);

		/// <summary>
		/// Sets the penumbra angle of this SpotLight instance. The penumbra angle describes the
		/// angle from the light's direction vector to the maximum extent of an imaginary inner
		/// cone such that for all points within said inner cone, angular attenuation has no effect
		/// on the luminance.
		/// 
		/// Necessarily, PenumbraAngle <= UmbraAngle (and thus cos(PenumbraAngle) >= cos(UmbraAngle)).
		/// 
		/// This value is sometimes less formally referred to as the "inner angle" of the spotlight.
		/// </summary>
		/// <param name="penumbraAngle">
		/// - The penumbra/inner angle, in radians, of the spotlight.
		/// </param>
		void SetPenumbraAngle(const float penumbraAngle);

	private:
		bool IsGPUSceneDataDirty() const;
		void MarkGPUSceneDataAsDirty();

		void CheckForRotationChange(const TransformComponent& transformComponent);
		void CheckForTranslationChange(const TransformComponent& transformComponent);

		void UpdateGPUSceneBufferData();

		std::uint32_t GetSpotLightBufferIndex() const;

	private:
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::SpotLight> mSpotLightBufferSubAllocation;
		LightDescriptorUpdater<LightID::SPOTLIGHT> mLightDescriptorUpdater;
		SpotLightInfo mSpotLightInfo;
		Math::Float3 mCachedDirectionWS;
		Math::Float3 mCachedTranslation;
		Float3 mLightDirectionLS;
		bool mIsGPUSceneDataDirty;
	};
}
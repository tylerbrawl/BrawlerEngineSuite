module;

export module Brawler.PointLight;
import Brawler.SceneNode;
import Brawler.LightDescriptorUpdater;
import Brawler.LightID;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	class PointLight final : public SceneNode
	{
	private:
		struct PointLightInfo
		{
			Math::Float3 LightColor;
			float LuminousIntensityInCandelas;
			float MaxDistanceInMeters;
		};

	public:
		PointLight();

		PointLight(const PointLight& rhs) = delete;
		PointLight& operator=(const PointLight& rhs) = delete;

		PointLight(PointLight&& rhs) noexcept = default;
		PointLight& operator=(PointLight&& rhs) noexcept = default;

		void LateUpdate(const float dt) override;

		/// <summary>
		/// Sets the luminous intensity, in Candelas (cd), of the PointLight instance.
		/// 
		/// NOTE: Frostbite exposes luminous power (in lumens, or lm) to artists and converts
		/// these values to luminous intensity for point lights as follows:
		///
		/// LuminousIntensity = LuminousPower lumens / (4 * PI steradians) = (LuminousPower / (4 * PI)) lm/sr = (LuminousPower / (4 * PI)) cd
		/// </summary>
		/// <param name="intensityInCandelas">
		/// - The luminous intensity, in Candelas (cd), of the light.
		/// </param>
		void SetLuminousIntensity(const float intensityInCandelas);

		/// <summary>
		/// Sets the color of this PointLight instance expressed as an RGB triple in (linear) sRGB 
		/// color space.
		/// 
		/// Future work would be to allow this value to be computed from, e.g., color temperature.
		/// </summary>
		/// <param name="lightColor">
		/// - The color of the light as an RGB triple in (linear) sRGB color space.
		/// </param>
		void SetLightColor(const Math::Float3 lightColor);

		/// <summary>
		/// Sets the maximum distance, in meters, which this PointLight instance reaches. If
		/// one were to imagine the point light as a sphere, then this value would be the sphere's
		/// radius.
		/// 
		/// *NOTE*: The maximum distance of a PointLight instance *MUST* be at least 1 cm, which is
		/// the "size" of a punctual light in the Brawler Engine. An assert is fired in Debug builds
		/// if this is not the case.
		/// </summary>
		/// <param name="maxDistanceInMeters">
		/// - The maximum distance, in meters, which this point light reaches.
		/// </param>
		void SetMaximumLightDistance(const float maxDistanceInMeters);

	private:
		bool IsGPUSceneDataDirty() const;
		void MarkGPUSceneDataAsDirty();

		void CheckForTranslationChange();

		void UpdateGPUSceneBufferData();

		std::uint32_t GetPointLightBufferIndex() const;

	private:
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::PointLight> mPointLightBufferSubAllocation;
		LightDescriptorUpdater<LightID::POINT_LIGHT> mLightDescriptorUpdater;
		PointLightInfo mPointLightInfo;
		Math::Float3 mCachedTranslation;
		bool mIsGPUSceneDataDirty;
	};
}
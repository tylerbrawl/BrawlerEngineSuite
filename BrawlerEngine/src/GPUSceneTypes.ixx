module;
#include <cstdint>
#include <DirectXMath/DirectXMath.h>

export module Brawler.GPUSceneTypes;
import Util.HLSL;

// NOTE: For some of these types, you will see that their corresponding HLSL structure has bit packing,
// while their C++ types either combine them into a single member or make the entire type a type alias
// for a primitive type. The reflection used by the Brawler Engine cannot correctly infer the sizes of 
// members with bit packing, so we don't use it on this end. 
// 
// (It also doesn't help that the DirectX specifications don't specify how bit packing is implemented 
// - that is, where individual bit fields are located in memory. I'd like to assume that HLSL bit 
// packing follows the same rules as MSVC C++ bit packing, but I have not confirmed this.)

export namespace Brawler
{
	namespace GPUSceneTypes
	{
		struct PackedStaticVertex
		{
			DirectX::XMFLOAT4 PositionAndTangentFrame;

			DirectX::XMFLOAT2 UVCoords;
			DirectX::XMUINT2 __Pad0;
		};

		struct ModelInstanceTransformData
		{
			DirectX::XMFLOAT4X3 CurrentFrameWorldMatrix;
			DirectX::XMFLOAT4X3 CurrentFrameInverseWorldMatrix;

			DirectX::XMFLOAT4X3 PreviousFrameWorldMatrix;
			DirectX::XMFLOAT4X3 PreviousFrameInverseWorldMatrix;
		};

		struct ViewTransformData
		{
			DirectX::XMFLOAT4X4 CurrentFrameViewProjectionMatrix;
			DirectX::XMFLOAT4X4 CurrentFrameInverseViewProjectionMatrix;

			DirectX::XMFLOAT4X4 PreviousFrameViewProjectionMatrix;
			DirectX::XMFLOAT4X4 PreviousFrameInverseViewProjectionMatrix;

			DirectX::XMFLOAT4 CurrentFrameViewSpaceQuaternion;

			/// <summary>
			/// This is the origin of the view space coordinate system with respect
			/// to the world space coordinate system for the current frame. 
			/// 
			/// This value can be used to get the view's position in world space.
			/// To use it in the construction of a view matrix, however, this value
			/// must be negated.
			/// </summary>
			DirectX::XMFLOAT3 CurrentFrameViewOriginWS;

			std::uint32_t __Pad0;

			DirectX::XMFLOAT4 PreviousFrameViewSpaceQuaternion;

			/// <summary>
			/// This is the origin of the view space coordinate system with respect
			/// to the world space coordinate system for the previous frame. 
			/// 
			/// This value can be used to get the view's position in world space.
			/// To use it in the construction of a view matrix, however, this value
			/// must be negated.
			/// </summary>
			DirectX::XMFLOAT3 PreviousFrameViewOriginWS;

			std::uint32_t __Pad1;
		};

		struct ViewDimensionsData
		{
			DirectX::XMUINT2 ViewDimensions;
			DirectX::XMFLOAT2 InverseViewDimensions;
		};

		using PackedModelInstanceDescriptor = std::uint32_t;
		using PackedViewDescriptor = std::uint32_t;

		struct MeshDescriptor
		{
			DirectX::XMFLOAT3 CurrentFrameAABBMin;
			std::uint32_t MaterialDescriptorIndex;

			DirectX::XMFLOAT3 CurrentFrameAABBMax;
			std::uint32_t IndexBufferSRVIndex;
		};

		struct MaterialDescriptor
		{
			std::uint32_t BaseColorTextureSRVIndex;
			std::uint32_t NormalMapSRVIndex;
			std::uint32_t RoughnessTextureSRVIndex;

			// TODO: This should probably be moved into a separate channel of a
			// different texture.
			std::uint32_t MetallicTextureSRVIndex;
		};

		using PackedLightDescriptor = std::uint32_t;

		enum class LightType
		{
			POINT_LIGHT = 0,
			SPOTLIGHT = 1
		};

		struct PointLight
		{
			/// <summary>
			/// This is the position, in world space, at which the point light is located
			/// in the scene.
			/// </summary>
			DirectX::XMFLOAT3 PositionWS;

			/// <summary>
			/// This is (1.0f / MaxDistance)^2, where MaxDistance is the maximum distance, in meters, 
			/// which the light reaches. If one were to imagine the point light as a sphere, then
			/// MaxDistance would be the sphere's radius.
			///
			/// *NOTE*: MaxDistance *MUST* be at least 1 cm, which is the "size" of a punctual
			/// light in the Brawler Engine.
			/// </summary>
			float InverseMaxDistanceSquared;

			/// <summary>
			/// This is the color of the light expressed as an RGB triple in (linear) sRGB color
			/// space. The value is scaled by the luminous intensity, in Candelas (cd), of the
			/// light.
			///
			/// NOTE: Frostbite exposes luminous power (in lumens, or lm) to artists and converts
			/// these values to luminous intensity for point lights as follows:
			///
			/// LuminousIntensity = LuminousPower lumens / (4 * PI steradians) = (LuminousPower / (4 * PI)) lm/sr = (LuminousPower / (4 * PI)) cd
			///
			/// (Future work would be to compute this value from, e.g., color temperature,
			/// but such conversions should never have to happen on the GPU.)
			/// </summary>
			DirectX::XMFLOAT3 ScaledLightColor;

			std::uint32_t __Pad0;
		};

		struct SpotLight
		{
			/// <summary>
			/// This is the position, in world space, at which the spotlight is located
			/// in the scene.
			/// </summary>
			DirectX::XMFLOAT3 PositionWS;

			/// <summary>
			/// This is (1.0f / MaxDistance)^2, where MaxDistance is the maximum distance, in meters, 
			/// which the light reaches. If one were to imagine the spotlight as a cone, then
			/// MaxDistance would be the magnitude of the *UNNORMALIZED* DirectionWS vector.
			///
			/// *NOTE*: MaxDistance *MUST* be at least 1 cm, which is the "size" of a punctual
			/// light in the Brawler Engine.
			/// </summary>
			float InverseMaxDistanceSquared;

			/// <summary>
			/// This is the color of the light expressed as an RGB triple in (linear) sRGB color
			/// space. The value is scaled by the luminous intensity, in Candelas (cd), of the
			/// light.
			///
			/// NOTE: Frostbite exposes luminous power (in lumens, or lm) to artists and converts
			/// these values to luminous intensity for spotlights as follows:
			///
			/// LuminousIntensity = LuminousPower lumens / PI steradians = (LuminousPower / PI) lm/sr = (LuminousPower / PI) cd
			///
			/// (Future work would be to compute this value from, e.g., color temperature,
			/// but such conversions should never have to happen on the GPU.)
			/// </summary>
			DirectX::XMFLOAT3 ScaledLightColor;

			/// <summary>
			/// This is the value of cos(UmbraAngle), where UmbraAngle describes the angle from the vector
			/// DirectionWS to the maximum extent of the cone bounding volume of the spotlight.
			///
			/// This value is sometimes less formally referred to as the "outer angle" of the spotlight.
			/// </summary>
			float CosineUmbraAngle;

			/// <summary>
			///	This is the (normalized) direction vector of the spotlight in world space. The direction
			/// vector describes the orientation of the spotlight.
			/// </summary>
			DirectX::XMFLOAT3 DirectionWS;

			/// <summary>
			/// This is the value of cos(PenumbraAngle), where PenumbraAngle describes the angle from the
			/// vector DirectionWS to the maximum extent of an imaginary inner cone such that for all
			/// points within said inner cone, angular attenuation has no effect on the luminance.
			///
			/// Necessarily, PenumbraAngle <= UmbraAngle (and thus CosinePenumbraAngle >= CosineUmbraAngle).
			///
			/// This value is sometimes less formally referred to as the "inner angle" of the spotlight.
			/// </summary>
			float CosinePenumbraAngle;
		};
	}
}
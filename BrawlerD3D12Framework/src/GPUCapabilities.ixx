module;
#include <cstdint>
#include <cstddef>

export module Brawler.D3D12.GPUCapabilities;
import Brawler.CompositeEnum;

export namespace Brawler
{
	namespace D3D12
	{
		enum class ResourceHeapTier
		{
			TIER_1,
			TIER_2
		};

		enum class ResourceBindingTier
		{
			TIER_1,
			TIER_2,
			TIER_3
		};

		enum class TypedUAVFormat
		{
			// Guaranteed - These formats are guaranteed to always support typed UAV loads
			// on all D3D12 hardware (and D3D11.3 hardware).

			R32_FLOAT,
			R32_UINT,
			R32_SINT,

			// ---------------------------------------------------------------------------------

			// Supported as a Set - If any one of these formats supports typed UAV loads, then
			// all of them will.

			R32G32B32A32_FLOAT,
			R32G32B32A32_UINT,
			R32G32B32A32_SINT,
			R16G16B16A16_FLOAT,
			R16G16B16A16_UINT,
			R16G16B16A16_SINT,
			R8G8B8A8_UNORM,
			R8G8B8A8_UINT,
			R8G8B8A8_SINT,
			R16_FLOAT,
			R16_UINT,
			R16_SINT,
			R8_UNORM,
			R8_UINT,
			R8_SINT,

			// ---------------------------------------------------------------------------------

			// Individually Supported - These formats need to be individually checked for support.

			R16G16B16A16_UNORM,
			R16G16B16A16_SNORM,
			R32G32_FLOAT,
			R32G32_UINT,
			R32G32_SINT,
			R10G10B10A2_UNORM,
			R10G10B10A2_UINT,
			R11G11B10_FLOAT,
			R8G8B8A8_SNORM,
			R16G16_FLOAT,
			R16G16_UNORM,
			R16G16_UINT,
			R16G16_SNORM,
			R16G16_SINT,
			R8G8_UNORM,
			R8G8_UINT,
			R8G8_SNORM,
			R8G8_SINT,
			R16_UNORM,
			R16_SNORM,
			R8_SNORM,
			A8_UNORM,
			B5G6R5_UNORM,
			B5G5R5A1_UNORM,
			B4G4R4A4_UNORM,

			COUNT_OR_ERROR
		};

		struct GPUCapabilities
		{
			ResourceHeapTier GPUResourceHeapTier;
			ResourceBindingTier GPUResourceBindingTier;
			Brawler::CompositeEnum<TypedUAVFormat> SupportedTypedUAVLoadFormats;
			std::uint32_t MaxGPUVirtualAddressBitsPerProcess;
			std::size_t DedicatedVideoMemorySizeInBytes;
		};
	}
}
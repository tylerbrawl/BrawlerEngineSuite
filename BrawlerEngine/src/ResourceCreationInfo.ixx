module;
#include <cstdint>
#include <cassert>
#include <optional>
#include "DxDef.h"

export module Brawler.ResourceCreationInfo;
import Util.Math;

export namespace Brawler
{
	class I_GPUResource;
}

export namespace Brawler
{
	enum class D3DHeapType : std::uint8_t
	{
		// This corresponds to D3D12_HEAP_TYPE_DEFAULT.
		NO_CPU_ACCESS,

		// This corresponds to D3D12_HEAP_TYPE_UPLOAD.
		CPU_WRITE_ONLY,

		// This corresponds to D3D12_HEAP_TYPE_READBACK.
		CPU_READ_ONLY,

		COUNT_OR_ERROR
	};

	// Provide a type alias for convenience. The idea is that this name makes more
	// sense in certain contexts than D3DHeapType does.
	using D3DResourceAccessMode = D3DHeapType;

	D3DHeapType GetHeapTypeFromHeapDescription(const D3D12_HEAP_DESC& heapDesc)
	{
		switch (heapDesc.Properties.Type)
		{
		case D3D12_HEAP_TYPE_DEFAULT:
			return D3DHeapType::NO_CPU_ACCESS;

		case D3D12_HEAP_TYPE_UPLOAD:
			return D3DHeapType::CPU_WRITE_ONLY;

		case D3D12_HEAP_TYPE_READBACK:
			return D3DHeapType::CPU_READ_ONLY;

		default:
		{
			assert(false && "ERROR: Custom heap types are not supported!");
			return D3DHeapType::COUNT_OR_ERROR;
		}
		}
	}

	// This enumeration is used to separate heaps based on whether they contain one
	// of the following resource types:
	//
	//   - Buffers
	//   - Textures which are *NOT* used as a render target or a depth/stencil buffer
	//   - Textures which *ARE* used as a render target or a depth/stencil buffer
	//
	// This helps ensure compatibility with Heap Tier 1 devices while also not breaking
	// compatibility with Heap Tier 2 devices.
	//
	// In addition, the texture classifications are further separated based on whether
	// or not the heaps will contain MSAA textures. This allows us to account for the
	// alignment differences between the two.
	enum class AllowedD3DResourceType
	{
		BUFFERS,

		RT_DS_TEXTURES_NO_MSAA,
		RT_DS_TEXTURES_MSAA,

		NON_RT_DS_TEXTURES_NO_MSAA,
		NON_RT_DS_TEXTURES_MSAA,

		COUNT_OR_ERROR
	};

	struct D3DHeapInfo
	{
		// This represents the type of resource which can be stored in the heap.
		AllowedD3DResourceType AllowedResourceType;

		// This represents the default alignment of resources for a given heap.
		std::uint32_t DefaultAlignment;

		// This represents the small alignment of resources for a given heap,
		// if such an alignment is ever allowed.
		std::optional<std::uint32_t> SmallAlignment;

		// This represents the required alignment of the heap itself.
		std::uint32_t HeapAlignment;
	};

	constexpr D3DHeapInfo GetHeapInfoFromAllowedResourceType(const AllowedD3DResourceType allowedType)
	{
		// Buffers are always aligned to 64 KB boundaries.
		// 
		// Textures which will be used as render targets or depth/stencil buffers can
		// never have small alignment, so they must always use their default values
		// (64 KB for non-MSAA textures, 4 MB for MSAA textures).
		//
		// Textures which will never be used as render targets or depth/stencil buffers
		// *MAY* have small alignment (4 KB for non-MSAA textures, 64 KB for MSAA
		// textures), but this is not guaranteed.
		//
		// Lastly, heap alignment is either 64 KB if the heap contains no MSAA textures
		// or 4 MB if it does.

		switch (allowedType)
		{
		case AllowedD3DResourceType::BUFFERS:
		case AllowedD3DResourceType::RT_DS_TEXTURES_NO_MSAA:
			return D3DHeapInfo{ allowedType, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, std::optional<std::uint32_t>{}, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT };

		case AllowedD3DResourceType::RT_DS_TEXTURES_MSAA:
			return D3DHeapInfo{ allowedType, D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT, std::optional<std::uint32_t>{}, D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT };

		case AllowedD3DResourceType::NON_RT_DS_TEXTURES_NO_MSAA:
			return D3DHeapInfo{ allowedType, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT };

		case AllowedD3DResourceType::NON_RT_DS_TEXTURES_MSAA:
			return D3DHeapInfo{ allowedType, D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT, D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT, D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT };

		default:
			assert(false && "ERROR: An unspecified Brawler::AllowedD3DResourceType was specified for Brawler::GetHeapInfoFromAllowedResourceType()!");
			return D3DHeapInfo{};
		}
	}

	constexpr AllowedD3DResourceType GetAllowedResourceTypeFromResourceDescription(const Brawler::D3D12_RESOURCE_DESC& resourceDesc)
	{
		assert(resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_UNKNOWN && "ERROR: An unknown resource dimension was provided when trying to determine the type of a D3D12 resource!");
		
		if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
			return AllowedD3DResourceType::BUFFERS;

		bool isMSAAEnabled = (resourceDesc.SampleDesc.Count != 1 || resourceDesc.SampleDesc.Quality != 0);
		bool isRTDSTexture = ((resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || (resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));

		if (isRTDSTexture)
			return (isMSAAEnabled ? AllowedD3DResourceType::RT_DS_TEXTURES_MSAA : AllowedD3DResourceType::RT_DS_TEXTURES_NO_MSAA);

		return (isMSAAEnabled ? AllowedD3DResourceType::NON_RT_DS_TEXTURES_MSAA : AllowedD3DResourceType::NON_RT_DS_TEXTURES_NO_MSAA);
	}

	struct ResourceCreationInfo
	{
		// This signifies the type of access the CPU has to this resource. Internally,
		// this value is used to define which heap type the resource can be created in.
		D3DResourceAccessMode AccessMode;

		// This is the resource allocation information for the resource.
		D3D12_RESOURCE_ALLOCATION_INFO AllocationInfo;

		// This is the description of the resource.
		Brawler::D3D12_RESOURCE_DESC ResourceDesc;

		// If this field has a valid value, then it is the optimized clear value specified
		// during the creation of the resource. See the MSDN article on
		// ID3D12Device8::CreatePlacedResource1() for more information.
		std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue;

		// This is the initial resource state of the resource. There are some restrictions
		// which must be met, which are defined as follows:
		//
		//   - Resources created with D3DResourceAccessMode::CPU_WRITE_ONLY *MUST* start in
		//     the D3D12_RESOURCE_STATE_GENERIC_READ state.
		//   - Resources created with D3DResourceAccessMode::CPU_READ_ONLY *MUST* start in the
		//     D3D12_RESOURCE_STATE_COPY_DEST state.
		D3D12_RESOURCE_STATES InitialState;

		// This is a reference to the resource which is to be initialized.
		I_GPUResource* Resource;
	};

	constexpr D3D12_HEAP_DESC GetHeapDescriptionFromResourceCreationInfo(const ResourceCreationInfo& creationInfo)
	{
		D3D12_HEAP_DESC heapDesc{};
		heapDesc.SizeInBytes = Util::Math::GetNextPowerOfTwo(creationInfo.AllocationInfo.SizeInBytes);

		switch (creationInfo.AccessMode)
		{
		case D3DResourceAccessMode::NO_CPU_ACCESS:
		{
			heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
			break;
		}

		case D3DResourceAccessMode::CPU_WRITE_ONLY:
		{
			heapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
			break;
		}

		case D3DResourceAccessMode::CPU_READ_ONLY:
		{
			heapDesc.Properties.Type = D3D12_HEAP_TYPE_READBACK;
			break;
		}

		default:
		{
			assert(false && "ERROR: An undefined ResourceCreationInfo was specified in Brawler::GetHeapDescriptionFromResourceCreationInfo()!");
			return heapDesc;
		}
		}

		heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		const AllowedD3DResourceType resourceType{ GetAllowedResourceTypeFromResourceDescription(creationInfo.ResourceDesc) };
		
		if (resourceType == AllowedD3DResourceType::RT_DS_TEXTURES_MSAA || resourceType == AllowedD3DResourceType::NON_RT_DS_TEXTURES_MSAA)
			heapDesc.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
		else
			heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

		switch (resourceType)
		{
		case AllowedD3DResourceType::BUFFERS:
		{
			heapDesc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			break;
		}

		case AllowedD3DResourceType::RT_DS_TEXTURES_MSAA:
		case AllowedD3DResourceType::RT_DS_TEXTURES_NO_MSAA:
		{
			heapDesc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
			break;
		}

		case AllowedD3DResourceType::NON_RT_DS_TEXTURES_MSAA:
		case AllowedD3DResourceType::NON_RT_DS_TEXTURES_NO_MSAA:
		{
			heapDesc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
			break;
		}

		default:
		{
			assert(false && "ERROR: An undefined ResourceCreationInfo was specified in Brawler::GetHeapDescriptionFromResourceCreationInfo()!");
			return heapDesc;
		}
		}

		return heapDesc;
	}
}
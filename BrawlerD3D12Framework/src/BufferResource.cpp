module;
#include <optional>
#include "DxDef.h"

module Brawler.D3D12.BufferResource;
import Brawler.D3D12.BufferResourceInitializationInfo;
import Brawler.D3D12.GPUResourceInitializationInfo;

namespace
{
	constexpr Brawler::D3D12::GPUResourceInitializationInfo ConvertBufferResourceInitializationInfo(const Brawler::D3D12::BufferResourceInitializationInfo& initInfo)
	{
		// The D3D12 API requires that buffers have a specific set of values for the majority of the
		// fields in their respective D3D12_RESOURCE_DESC structures. These are defined as follows:
		constexpr Brawler::D3D12_RESOURCE_DESC REQUIRED_BUFFER_DESC{
			.Dimension = D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
			.Width{},
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
			.SampleDesc{
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT::D3D12_TEXTURE_LAYOUT_ROW_MAJOR,

			// *IMPORTANT UPDATE*: The MSDN lied again! As it turns out, buffers cannot be used as either
			// a render target (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) or with an unordered access
			// view (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) if it is being created in either a
			// D3D12_HEAP_TYPE_UPLOAD or a D3D12_HEAP_TYPE_READBACK heap. Failing to follow this rule
			// results in a debug layer error (State Creation Error #638). I'll leave the original
			// comment below for historical purposes.
			//
			// The MSDN states that we must still correctly fill out the Flags parameter, even though
			// D3D12_RESOURCE_FLAGS are meant to control texture properties. However, it also states
			// that we can "use the most amount of capability support without concern about the
			// efficiency impact on buffers," so we'll just specify the most powerful flags which make
			// sense.
			.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,

			.SamplerFeedbackMipRegion{}
		};

		Brawler::D3D12_RESOURCE_DESC bufferDesc{ REQUIRED_BUFFER_DESC };
		bufferDesc.Width = initInfo.SizeInBytes;

		// Buffers created in DEFAULT heaps are implicitly promoted on their first use. Thus, it makes
		// sense to start them in the COMMON state. Indeed, PIX shows that regardless of what initial
		// state you provide for a buffer during resource creation, its initial state is always set to
		// the COMMON state internally.
		D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

		if (initInfo.HeapType != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT)
		{
			bufferDesc.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_NONE;

			if (initInfo.HeapType == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
				initialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ;
			else
				initialResourceState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST;
		}

		return Brawler::D3D12::GPUResourceInitializationInfo{
			.ResourceDesc{std::move(bufferDesc)},
			.InitialResourceState = initialResourceState,
			.HeapType = initInfo.HeapType
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		BufferResource::BufferResource(const BufferResourceInitializationInfo& initInfo) :
			I_GPUResource(ConvertBufferResourceInitializationInfo(initInfo)),
			mSubAllocationManager(*this, initInfo.SizeInBytes)
		{}

		void BufferResource::ExecutePostD3D12ResourceInitializationCallback()
		{
			mSubAllocationManager.OnD3D12ResourceInitialized();
		}

		bool BufferResource::CanAliasBeforeUseOnGPU() const
		{
			// When using a buffer in an upload heap to transfer data from the CPU to the GPU, we
			// need to make sure that the data is uploaded from the CPU and present for the GPU to
			// read by the time it executes the command.
			//
			// If we allow other resources to alias this resource before this resource is ever
			// used, then any data written on the CPU timeline will be overwritten on the GPU
			// timeline before the GPU can ever get the chance to read what the CPU wrote.
			//
			// To prevent this from happening, we disallow aliasing resources located in an upload
			// heap until after their contents have finished being used by the GPU.

			return (GetHeapType() != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
		}

		bool BufferResource::CanAliasAfterUseOnGPU() const
		{
			// Strictly speaking, we also would not want to alias buffers created in a readback
			// heap after they are accessed by the GPU. This is because we do not want this data
			// to be overwritten before it can be accessed by the CPU.
			//
			// In practice, since aliasing is only done between transient resources, and since it
			// would not be helpful to make a transient buffer in a readback heap, this should
			// never really occur. Still, this is the technically correct thing to do.

			return (GetHeapType() != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK);
		}
	}
}
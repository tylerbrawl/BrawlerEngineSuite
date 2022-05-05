module;
#include <cassert>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceDescriptors.DescriptorTableBuilder;
import Brawler.D3D12.GPUResourceDescriptors.PerFrameDescriptorTable;
import Util.Engine;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceDescriptors.GPUResourceDescriptorHeap;

export namespace Brawler
{
	namespace D3D12
	{
		class DescriptorTableBuilder
		{
		private:
			friend class GPUResourceDescriptorHeap;

		public:
			explicit DescriptorTableBuilder(const std::uint32_t tableSizeInDescriptors);

			DescriptorTableBuilder(const DescriptorTableBuilder& rhs) = delete;
			DescriptorTableBuilder& operator=(const DescriptorTableBuilder& rhs) = delete;

			DescriptorTableBuilder(DescriptorTableBuilder&& rhs) noexcept = default;
			DescriptorTableBuilder& operator=(DescriptorTableBuilder&& rhs) noexcept = default;

			void CreateConstantBufferView(const std::uint32_t index, I_GPUResource& resource);
			void CreateNullConstantBufferView(const std::uint32_t index);

			void CreateShaderResourceView(const std::uint32_t index, I_GPUResource& resource);

			/// <summary>
			/// Creates an unordered access view (UAV) without a UAV counter.
			/// </summary>
			/// <param name="index">
			/// - The index into the descriptor table at which the created descriptor can later
			///   be accessed.
			/// </param>
			/// <param name="resource">
			/// - The GPU resource for which a UAV will be created.
			/// </param>
			void CreateUnorderedAccessView(const std::uint32_t index, I_GPUResource& resource);

			/// <summary>
			/// Creates an unordered access view (UAV) with a UAV counter. In addition to the
			/// constraint that only buffers may have UAV counters, there are other conditions which
			/// must be met; see the MSDN at 
			/// https://docs.microsoft.com/en-gb/windows/win32/direct3d12/uav-counters#using-uav-counters
			/// for more details.
			/// </summary>
			/// <param name="index">
			/// - The index into the descriptor table at which the created descriptor can later
			///   be accessed.
			/// </param>
			/// <param name="bufferResource">
			/// - The GPU resource for which a UAV will be created. Note that only buffers can have a 
			///   UAV counter, and the created UAV must be typeless. (This does not meant that it cannot 
			///   be used for StructuredBuffers; rather, it means that it cannot be used with typed UAV 
			///   loads.)
			/// </param>
			/// <param name="counterResource">
			/// - The GPU resource within which the UAV counter for bufferResource will be created.
			///   Note that only buffers can be used as UAV counters. This can be the same resource as 
			///   that which is specified by bufferResource; in that case, one should make sure that
			///   the buffer is large enough to hold all of its normal data *AND* a UAV counter. (The
			///   size of a UAV counter is 32 bits, i.e., sizeof(std::uint32_t).)
			/// </param>
			void CreateUnorderedAccessView(const std::uint32_t index, I_GPUResource& bufferResource, I_GPUResource& counterResource);

			template <D3D12_UAV_DIMENSION UAVDimension>
			void CreateNullUnorderedAccessView(const std::uint32_t index);

			PerFrameDescriptorTable FinalizeDescriptorTable() const;

			/// <summary>
			/// Retrieves the size of the descriptor table.
			/// 
			/// NOTE: The returned value does *NOT* equal the number of descriptors which were
			/// created via calls to CreateConstantBufferView(), CreateShaderResourceView(), or
			/// CreateUnorderedAccessView()! It only describes the number of descriptors which
			/// the table is capable of supporting.
			/// </summary>
			/// <returns>
			/// The function returns the size of the descriptor table.
			/// </returns>
			std::uint32_t GetDescriptorTableSize() const;

		private:
			CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors = 0) const;

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> mStagingHeap;
			std::uint32_t mNumDescriptors;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_UAV_DIMENSION UAVDimension>
		struct NullUAVInfo
		{
			static_assert(sizeof(UAVDimension) != sizeof(UAVDimension), "ERROR: An explicit template instantiation of Brawler::D3D12::NullUAVInfo was never provided for a given D3D12_UAV_DIMENSION! (See DescriptorTableBuilder.ixx.)");
		};

		template <>
		struct NullUAVInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER>
		{
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC UAV_DESC{
				.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER,
				.Buffer{
					.FirstElement = 0,
					.NumElements = 0,

					// We need to specify a value for StructureByteStride; otherwise, we would have to specify
					// a format for the Format field of the UAV description. This would mean creating a typed
					// UAV, which may or may not be supported on a device. Let's not risk anything.
					.StructureByteStride = sizeof(std::uint32_t),

					.CounterOffsetInBytes = 0
				}
			};
		};

		template <>
		struct NullUAVInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D>
		{
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC UAV_DESC{
				.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D,
				.Texture1D{
					.MipSlice = 0
				}
			};
		};

		template <>
		struct NullUAVInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1DARRAY>
		{
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC UAV_DESC{
				.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1DARRAY,
				.Texture1DArray{
					.MipSlice = 0,
					.FirstArraySlice = 0,
					.ArraySize = 0
				}
			};
		};

		template <>
		struct NullUAVInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D>
		{
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC UAV_DESC{
				.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D,
				.Texture2D{
					.MipSlice = 0,
					.PlaneSlice = 0
				}
			};
		};

		template <>
		struct NullUAVInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY>
		{
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC UAV_DESC{
				.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
				.Texture2DArray{
					.MipSlice = 0,
					.FirstArraySlice = 0,
					.ArraySize = 0,
					.PlaneSlice = 0
				}
			};
		};

		template <>
		struct NullUAVInfo<D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE3D>
		{
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC UAV_DESC{
				.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN,
				.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE3D,
				.Texture3D{
					.MipSlice = 0,
					.FirstWSlice = 0,
					.WSize = 0
				}
			};
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <D3D12_UAV_DIMENSION UAVDimension>
		void DescriptorTableBuilder::CreateNullUnorderedAccessView(const std::uint32_t index)
		{
			Util::Engine::GetD3D12Device().CreateUnorderedAccessView(
				nullptr,
				nullptr,
				&(NullUAVInfo<UAVDimension>::UAV_DESC),
				GetCPUDescriptorHandle(index)
			);
		}
	}
}

// I don't know why, but using a module implementation unit for DescriptorTableBuilder causes CL.exe to crash
// with an undefined error code. To workaround this, we need to define the implementation in the module
// interface unit.

namespace Brawler
{
	namespace D3D12
	{
		DescriptorTableBuilder::DescriptorTableBuilder(const std::uint32_t tableSizeInDescriptors) :
			mStagingHeap(nullptr),
			mNumDescriptors(tableSizeInDescriptors)
		{
			// Create the non-shader-visible descriptor heap for staging the descriptors.
			const D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = tableSizeInDescriptors,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				.NodeMask = 0
			};

			CheckHRESULT(Util::Engine::GetD3D12Device().CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mStagingHeap)));
		}

		void DescriptorTableBuilder::CreateConstantBufferView(const std::uint32_t index, I_GPUResource& resource)
		{
			assert(resource.GetResourceDescription().Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER && "ERROR: An attempt was made to create a CBV for a GPU resource which was not a buffer!");

			const std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> cbvDesc{ resource.CreateCBVDescription() };
			assert(cbvDesc.has_value() && "ERROR: An attempt was made to create a CBV for a GPU resource, but it did not provide a valid D3D12_CONSTANT_BUFFER_VIEW_DESC!");

			Util::Engine::GetD3D12Device().CreateConstantBufferView(&(*cbvDesc), GetCPUDescriptorHandle(index));
		}

		void DescriptorTableBuilder::CreateNullConstantBufferView(const std::uint32_t index)
		{
			// So, the MSDN says that we can create a null descriptor by specifying a proper view description
			// (in this case, a D3D12_CONSTANT_BUFFER_VIEW_DESC) with sane default values and setting the
			// pResource parameter of the function used for creating the descriptor (in this case, the
			// ID3D12Device::CreateConstantBufferView() function) to nullptr.
			//
			// Uh... What pResource parameter? I'm just going to set the BufferLocation field of the
			// D3D12_CONSTANT_BUFFER_VIEW_DESC to 0 (nullptr) and hope that I'm doing it right.

			static constexpr D3D12_CONSTANT_BUFFER_VIEW_DESC NULL_SRV_DESC{
				.BufferLocation = 0,
				.SizeInBytes = 0
			};

			Util::Engine::GetD3D12Device().CreateConstantBufferView(&NULL_SRV_DESC, GetCPUDescriptorHandle(index));
		}

		void DescriptorTableBuilder::CreateShaderResourceView(const std::uint32_t index, I_GPUResource& resource)
		{
			const std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDesc{ resource.CreateSRVDescription() };
			assert(srvDesc.has_value() && "ERROR: An attempt was made to create an SRV for a GPU resource, but it did not provide a valid D3D12_SHADER_RESOURCE_VIEW_DESC!");

			Util::Engine::GetD3D12Device().CreateShaderResourceView(&(resource.GetD3D12Resource()), &(*srvDesc), GetCPUDescriptorHandle(index));
		}

		void DescriptorTableBuilder::CreateUnorderedAccessView(const std::uint32_t index, I_GPUResource& resource)
		{
			const std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uavDesc{ resource.CreateUAVDescription() };
			assert(uavDesc.has_value() && "ERROR: An attempt was made to create a UAV for a GPU resource, but it did not provide a valid D3D12_UNORDERED_ACCESS_VIEW_DESC!");

			Util::Engine::GetD3D12Device().CreateUnorderedAccessView(
				&(resource.GetD3D12Resource()),
				nullptr,
				&(*uavDesc),
				GetCPUDescriptorHandle(index)
			);
		}

		void DescriptorTableBuilder::CreateUnorderedAccessView(const std::uint32_t index, I_GPUResource& bufferResource, I_GPUResource& counterResource)
		{
			const std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uavDesc{ bufferResource.CreateUAVDescription() };
			assert(uavDesc.has_value() && "ERROR: An attempt was made to create a UAV for a GPU resource, but it did not provide a valid D3D12_UNORDERED_ACCESS_VIEW_DESC!");

			// In Debug builds, we will perform various checks required by the D3D12 API when using a UAV counter.
			//
			// (TODO: Can we just let the D3D12 Debug Layer take care of this?)
			assert(uavDesc->ViewDimension == D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER && bufferResource.GetResourceDescription().Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER && "ERROR: Only buffer resources can have a UAV counter!");
			assert(uavDesc->Format == DXGI_FORMAT::DXGI_FORMAT_UNKNOWN && uavDesc->Buffer.StructureByteStride > 0 && "ERROR: Only typeless UAVs can have a UAV counter!");
			assert((uavDesc->Buffer.Flags & D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_RAW) == 0 && "ERROR: Raw buffers cannot have a UAV counter!");
			assert(counterResource.GetResourceDescription().Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER && "ERROR: Only buffers can be used as UAV counters!");
			assert((uavDesc->Buffer.CounterOffsetInBytes % D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT) == 0 && "ERROR: UAV counters must be placed at an offset which is a multiple of D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT!");

			Util::Engine::GetD3D12Device().CreateUnorderedAccessView(
				&(bufferResource.GetD3D12Resource()),
				&(counterResource.GetD3D12Resource()),
				&(*uavDesc),
				GetCPUDescriptorHandle(index)
			);
		}

		PerFrameDescriptorTable DescriptorTableBuilder::FinalizeDescriptorTable() const
		{
			return Util::Engine::GetGPUResourceDescriptorHeap().CreatePerFrameDescriptorTable(*this);
		}

		std::uint32_t DescriptorTableBuilder::GetDescriptorTableSize() const
		{
			return mNumDescriptors;
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorTableBuilder::GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors) const
		{
			return CD3DX12_CPU_DESCRIPTOR_HANDLE{ mStagingHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<std::int32_t>(offsetInDescriptors), Util::Engine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) };
		}
	}
}
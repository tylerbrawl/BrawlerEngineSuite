module;
#include <variant>
#include <optional>
#include <cassert>
#include <mutex>
#include "DxDef.h"

export module Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.PerFrameDescriptorTable;
import Brawler.OptionalRef;
import Brawler.D3D12.GPUResourceViews;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.UAVCounterSubAllocation;
import Brawler.D3D12.GPUCapabilities;
import Util.Engine;

namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceDescriptorHeap;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class DescriptorTableBuilder
		{
		private:
			friend class GPUResourceDescriptorHeap;

		private:
			struct SRVInfo
			{
				const I_GPUResource* GPUResourcePtr;
				D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			};

			struct UAVInfo
			{
				const I_GPUResource* GPUResourcePtr;

				/// <summary>
				/// If this member is left empty, then no UAV counter is added to the created
				/// UAV. Otherwise, the specified Brawler::D3D12Resource is used as the UAV
				/// counter.
				/// 
				/// Note that only buffers can have a UAV counter, and there are requirements
				/// which are validated by the D3D12 runtime when using UAV counters. More information
				/// about this can be found at 
				/// https://docs.microsoft.com/en-us/windows/win32/direct3d12/uav-counters#using-uav-counters.
				/// </summary>
				Brawler::OptionalRef<Brawler::D3D12Resource> UAVCounterResource;

				D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
			};

			using DescriptorInfoVariant = std::variant<std::monostate, D3D12_CONSTANT_BUFFER_VIEW_DESC, SRVInfo, UAVInfo>;

		public:
			DescriptorTableBuilder() = delete;
			explicit DescriptorTableBuilder(const std::uint32_t tableSizeInDescriptors);

			DescriptorTableBuilder(const DescriptorTableBuilder& rhs) = delete;
			DescriptorTableBuilder& operator=(const DescriptorTableBuilder& rhs) = delete;

			DescriptorTableBuilder(DescriptorTableBuilder&& rhs) noexcept;
			DescriptorTableBuilder& operator=(DescriptorTableBuilder&& rhs) noexcept;

			template <typename DataElementType>
			void CreateConstantBufferView(const std::uint32_t index, const ConstantBufferView<DataElementType> cbv);

			void NullifyConstantBufferView(const std::uint32_t index);

			template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
			void CreateShaderResourceView(const std::uint32_t index, const ShaderResourceView<Format, ViewDimension>& srv);

			template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
			void CreateUnorderedAccessView(const std::uint32_t index, const UnorderedAccessView<Format, ViewDimension>& uav);

			/// <summary>
			/// Sets the descriptor at the specified index within the descriptor table being built to a NULL 
			/// UAV. This is done to ensure correct behavior on devices with only resource binding tier 2 support.
			/// 
			/// If the device has resource binding tier 3 support, then this function does nothing. As such, this 
			/// function should always be called if a UAV in a descriptor table is left unused, regardless of the 
			/// user's device.
			/// 
			/// The function is templated because NULL descriptors must be created such that if the descriptor
			/// were not NULL, then it could potentially refer to a resource which could be used. For example,
			/// if a root parameter in a root signature asks for N UAVs and a shader uses these UAVs as
			/// RWTexture2D instances, then the NULL descriptor must contain dummy information for a
			/// D3D12_UAV_DIMENSION_TEXTURE2D resource. Otherwise, the behavior is undefined.
			/// 
			/// Sadly, the MSDN and DirectX specifications are quite vague as to what values need to be set in
			/// any given scenario, so we err on the side of caution be requiring both a DXGI_FORMAT and a
			/// D3D12_UAV_DIMENSION template parameter.
			/// </summary>
			/// <typeparam name="Format">
			/// - The DXGI_FORMAT of a resource which, had this UAV not been nullified, it would be in.
			/// </typeparam>
			/// <typeparam name="ViewDimension">
			/// - The type of resource which, had this UAV not been nullified, it would be describing.
			/// </typeparam>
			/// <param name="index">
			/// - The zero-based index from the start of the descriptor table at which the NULL UAV will be
			///   created.
			/// </param>
			template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
			void NullifyUnorderedAccessView(const std::uint32_t index);

			/// <summary>
			/// If this is the first time which this function is called for this DescriptorTableBuilder
			/// instance on a given frame, then it will allocate from the GPUResourceDescriptorHeap's per-frame
			/// descriptor segment and create the descriptors within it. After that, the return value
			/// of this function is the table which was created for the remainder of the frame.
			/// 
			/// The DescriptorTableBuilder::Create*View() functions do not immediately create D3D12
			/// descriptors; instead, they copy information which will be used to actually create
			/// the descriptors once DescriptorTableBuilder::GetDescriptorTable() is called. This
			/// allows for DescriptorTableBuilder instances to be created before the I_GPUResource
			/// instances which it is creating descriptors for have been assigned ID3D12Resource
			/// instances. For this reason, it is the caller's responsibility to ensure that
			/// I_GPUResource instances for which descriptors are to be made outlive this
			/// DescriptorTableBuilder instance.
			/// 
			/// Although this function returns a PerFrameDescriptorTable instance, it is valid to
			/// both retain this DescriptorTableBuilder instance and call this function again across
			/// multiple frames. A new PerFrameDescriptorTable instance is created on the first call
			/// of this function on any given frame. However, it is *NOT* valid to persist the
			/// created PerFrameDescriptorTable instances across multiple frames.
			/// 
			/// *NOTE*: To ensure that descriptors can be created, this function should only be
			/// called after all of the relevant I_GPUResource instances have had ID3D12Resource
			/// instances assigned to them. For both persistent and transient resources, this is guaranteed
			/// to be the case during RenderPass recording time on the CPU timeline; that is, it
			/// is valid to call this function within a callback which is passed to
			/// RenderPass::SetRenderPassCommands().
			/// </summary>
			/// <returns>
			/// The function returns a PerFrameDescriptorTable which contains the descriptors
			/// described by this DescriptorTableBuilder.
			/// </returns>
			PerFrameDescriptorTable GetDescriptorTable();

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
			void CreateDescriptorTable();

			void CreateConstantBufferView(const std::uint32_t index, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvInfo);
			void CreateShaderResourceView(const std::uint32_t index, const SRVInfo& srvInfo);
			void CreateUnorderedAccessView(const std::uint32_t index, const UAVInfo& uavInfo);

			CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const std::uint32_t offsetInDescriptors = 0) const;

		private:
			Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> mStagingHeap;
			std::optional<PerFrameDescriptorTable> mDescriptorTable;
			std::vector<DescriptorInfoVariant> mDescriptorInfoArr;
			std::uint32_t mNumDescriptors;
			mutable std::mutex mTableCreationCritSection;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		// Even if we bind a NULL UAV descriptor to the pipeline, we still need to provide a UAV description
		// which could theoretically be used to create an actual UAV. We define these descriptions here.
		
		template <DXGI_FORMAT ViewFormat, D3D12_UAV_DIMENSION Dimension>
		struct NullUAVDescInfo
		{};

		template <DXGI_FORMAT ViewFormat>
		struct NullUAVDescInfo<ViewFormat, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER>
		{
		public:
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC CreateNullUAVDescription()
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC nullUAVDesc{};
				nullUAVDesc.Format = ViewFormat;
				nullUAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER;
				nullUAVDesc.Buffer = D3D12_BUFFER_UAV{
					.FirstElement = 0,
					.NumElements = 0,

					// According to the MSDN, StructureByteStride can only be 0 when the buffer
					// SRV is *NOT* for a StructuredBuffer. A StructuredBuffer view both has a
					// non-zero StructureByteStride and a Format of DXGI_FORMAT_UNKNOWN.
					.StructureByteStride = (ViewFormat == DXGI_FORMAT::DXGI_FORMAT_UNKNOWN ? 1 : 0),

					.CounterOffsetInBytes = 0
				};

				return nullUAVDesc;
			}
		};

		template <DXGI_FORMAT ViewFormat>
		struct NullUAVDescInfo<ViewFormat, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D>
		{
		public:
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC CreateNullUAVDescription()
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC nullUAVDesc{};
				nullUAVDesc.Format = ViewFormat;
				nullUAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1D;
				nullUAVDesc.Texture1D = D3D12_TEX1D_UAV{
					.MipSlice = 0
				};

				return nullUAVDesc;
			}
		};

		template <DXGI_FORMAT ViewFormat>
		struct NullUAVDescInfo<ViewFormat, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1DARRAY>
		{
		public:
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC CreateNullUAVDescription()
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC nullUAVDesc{};
				nullUAVDesc.Format = ViewFormat;
				nullUAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
				nullUAVDesc.Texture1DArray = D3D12_TEX1D_ARRAY_UAV{
					.MipSlice = 0,
					.FirstArraySlice = 0,
					.ArraySize = 0
				};

				return nullUAVDesc;
			}
		};

		template <DXGI_FORMAT ViewFormat>
		struct NullUAVDescInfo<ViewFormat, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D>
		{
		public:
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC CreateNullUAVDescription()
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC nullUAVDesc;
				nullUAVDesc.Format = ViewFormat;
				nullUAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;
				nullUAVDesc.Texture2D = D3D12_TEX2D_UAV{
					.MipSlice = 0,
					.PlaneSlice = 0
				};

				return nullUAVDesc;
			}
		};

		template <DXGI_FORMAT ViewFormat>
		struct NullUAVDescInfo<ViewFormat, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY>
		{
		public:
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC CreateNullUAVDescription()
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC nullUAVDesc{};
				nullUAVDesc.Format = ViewFormat;
				nullUAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				nullUAVDesc.Texture2DArray = D3D12_TEX2D_ARRAY_UAV{
					.MipSlice = 0,
					.FirstArraySlice = 0,
					.ArraySize = 0,
					.PlaneSlice = 0
				};

				return nullUAVDesc;
			}
		};

		template <DXGI_FORMAT ViewFormat>
		struct NullUAVDescInfo<ViewFormat, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE3D>
		{
		public:
			static constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC CreateNullUAVDescription()
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC nullUAVDesc{};
				nullUAVDesc.Format = ViewFormat;
				nullUAVDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE3D;
				nullUAVDesc.Texture3D = D3D12_TEX3D_UAV{
					.MipSlice = 0,
					.FirstWSlice = 0,
					.WSize = 0
				};

				return nullUAVDesc;
			}
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <typename DataElementType>
		void DescriptorTableBuilder::CreateConstantBufferView(const std::uint32_t index, const ConstantBufferView<DataElementType> cbv)
		{
			assert(!mDescriptorTable.has_value() && "ERROR: DescriptorTableBuilder::CreateConstantBufferView() was called after DescriptorTableBuilder::GetDescriptorTable()!");
			assert(index < mDescriptorInfoArr.size());

			const std::scoped_lock<std::mutex> lock{ mTableCreationCritSection };
			
			mDescriptorInfoArr[index] = cbv.GetCBVDescription();
		}

		template <DXGI_FORMAT Format, D3D12_SRV_DIMENSION ViewDimension>
		void DescriptorTableBuilder::CreateShaderResourceView(const std::uint32_t index, const ShaderResourceView<Format, ViewDimension>& srv)
		{
			assert(!mDescriptorTable.has_value() && "ERROR: DescriptorTableBuilder::CreateShaderResourceView() was called after DescriptorTableBuilder::GetDescriptorTable()!");
			assert(index < mDescriptorInfoArr.size());

			const std::scoped_lock<std::mutex> lock{ mTableCreationCritSection };

			mDescriptorInfoArr[index] = SRVInfo{
				.GPUResourcePtr{ &(srv.GetGPUResource()) },
				.SRVDesc{ srv.CreateSRVDescription() }
			};
		}

		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		void DescriptorTableBuilder::CreateUnorderedAccessView(const std::uint32_t index, const UnorderedAccessView<Format, ViewDimension>& uav)
		{
			assert(!mDescriptorTable.has_value() && "ERROR: DescriptorTableBuilder::CreateUnorderedAccessView() was called after DescriptorTableBuilder::GetDescriptorTable()!");
			assert(index < mDescriptorInfoArr.size());

			const std::scoped_lock<std::mutex> lock{ mTableCreationCritSection };

			mDescriptorInfoArr[index] = UAVInfo{
				.GPUResourcePtr{ &(uav.GetGPUResource()) },
				.UAVCounterResource{ uav.GetUAVCounterResource() },
				.UAVDesc{ uav.CreateUAVDescription() }
			};
		}

		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		void DescriptorTableBuilder::NullifyUnorderedAccessView(const std::uint32_t index)
		{
			assert(!mDescriptorTable.has_value() && "ERROR: DescriptorTableBuilder::NullifyUnorderedAccessView() was called after DescriptorTableBuilder::GetDescriptorTable()!");
			assert(index < mDescriptorInfoArr.size());

			// Don't bother setting a NULL descriptor if we do not need to.
			if (Util::Engine::GetGPUCapabilities().GPUResourceBindingTier == ResourceBindingTier::TIER_3) [[likely]]
				return;

			// The MSVC refuses to let this be constexpr, even though it could/should be. There's some type
			// of bug with the compiler which causes it to write out more data than is necessary for some
			// structures, resulting in a buffer overflow. However, this bug only occurs in a constant-evaluated
			// context.
			const auto nullUAVDesc = NullUAVDescInfo<Format, ViewDimension>::CreateNullUAVDescription();

			const std::scoped_lock<std::mutex> lock{ mTableCreationCritSection };

			mDescriptorInfoArr[index] = UAVInfo{
				.GPUResourcePtr = nullptr,
				.UAVCounterResource{},
				.UAVDesc{ nullUAVDesc }
			};
		}
	}
}
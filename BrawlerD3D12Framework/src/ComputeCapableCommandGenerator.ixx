module;
#include <cassert>
#include <array>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.ComputeCapableCommandGenerator;
import Brawler.D3D12.GPUCommandContext;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.UnorderedAccessView;
import Util.Engine;
import Util.General;
import Brawler.D3D12.GPUResourceDescriptorHeap;
import Brawler.D3D12.DescriptorHandleInfo;
import Brawler.OptionalRef;
import Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedClass>
		class ComputeCapableCommandGenerator
		{
		protected:
			ComputeCapableCommandGenerator() = default;

		public:
			virtual ~ComputeCapableCommandGenerator() = default;

			ComputeCapableCommandGenerator(const ComputeCapableCommandGenerator& rhs) = delete;
			ComputeCapableCommandGenerator& operator=(const ComputeCapableCommandGenerator& rhs) = delete;

			ComputeCapableCommandGenerator(ComputeCapableCommandGenerator&& rhs) noexcept = default;
			ComputeCapableCommandGenerator& operator=(ComputeCapableCommandGenerator&& rhs) noexcept = default;

			void Dispatch(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY, const std::uint32_t numThreadGroupsZ) const;

			void Dispatch1D(const std::uint32_t numThreadGroups) const;
			void Dispatch2D(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY) const;
			void Dispatch3D(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY, const std::uint32_t numThreadGroupsZ) const;

			/// <summary>
			/// Given an unordered access view (UAV) which refers to one or multiple subresources of an
			/// I_GPUResource, this function will clear the portion of the resource viewed by the specified
			/// UAV. The value used to clear the resource differs based on the underlying type of the
			/// resource:
			/// 
			///   - If uavToClear refers to a buffer GPU resource, then the segment of buffer memory viewed
			///     by it is zeroed, as if by a call to SecureZeroMemory() from the Win32 API.
			/// 
			///   - If uavToClear refers to a texture GPU resource, then the clear value used is that which
			///     is returned by I_GPUResource::GetOptimizedClearValue(); this function is called on the
			///     I_GPUResource instance referred to by uavToClear. The function asserts if an empty
			///     std::optional instance is returned by I_GPUResource::GetOptimizedClearValue(), but only
			///     for texture GPU resources.
			/// 
			/// The relevant subresources are cleared on the *GPU* timeline, regardless of whether or not
			/// their corresponding I_GPUResource is located in an UPLOAD heap. If you need to immediately
			/// clear a segment of a buffer created in an UPLOAD heap on the CPU timeline, then use the
			/// appropriate functions of the relevant derived I_BufferSubAllocation class.
			/// 
			/// Unfortunately, the parameters of the corresponding D3D12 API function (that is,
			/// ID3D12GraphicsCommandList::ClearUnorderedAccessViewFloat()) require the creation of a
			/// separate non-shader-visible descriptor heap for the resource in addition to an allocation
			/// within the GPUResourceDescriptorHeap. Each call to this function thus results in the creation
			/// of a temporary ID3D12DescriptorHeap instance, along with several descriptor heap operations.
			/// 
			/// For that reason, calling this function may or may not be more efficient than simply clearing
			/// an unordered access view from within a shader. (In fact, some implementations of the D3D12
			/// API may implement the clear operation as a dispatch, anyways.)
			/// </summary>
			/// <typeparam name="Format">
			/// - The DXGI_FORMAT of the UAV referring to the portion of an I_GPUResource which is to be
			///   cleared. This value is allowed to be different from the format of the I_GPUResource,
			///   but it should be valid to cast between the two formats. This can be verified by calling
			///   Util::D3D12::IsUAVResourceCastLegal().
			/// </typeparam>
			/// <typeparam name="ViewDimension">
			/// - The D3D12_UAV_DIMENSION of the UAV referring to the portion of an I_GPUResource which is
			///   to be cleared.
			/// </typeparam>
			/// <param name="uavToClear">
			/// - The UAV referring to the portion of an I_GPUResource which is to be cleared.
			/// </param>
			template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
			void ClearUnorderedAccessView(const UnorderedAccessView<Format, ViewDimension>& uavToClear) const;

		private:
			Brawler::D3D12GraphicsCommandList& GetDerivedClassCommandList() const;
		};
	}
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedClass>
		void ComputeCapableCommandGenerator<DerivedClass>::Dispatch(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY, const std::uint32_t numThreadGroupsZ) const
		{
			assert(numThreadGroupsX <= D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION && "ERROR: Too many thread groups were dispatched in the X-dimension in a call to GPUCommandContext::Dispatch()!");
			assert(numThreadGroupsY <= D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION && "ERROR: Too many thread groups were dispatched in the Y-dimension in a call to GPUCommandContext::Dispatch()!");
			assert(numThreadGroupsZ <= D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION && "ERROR: Too many thread groups were dispatched in the Z-dimension in a call to GPUCommandContext::Dispatch()!");

			GetDerivedClassCommandList().Dispatch(numThreadGroupsX, numThreadGroupsY, numThreadGroupsZ);
		}

		template <typename DerivedClass>
		void ComputeCapableCommandGenerator<DerivedClass>::Dispatch1D(const std::uint32_t numThreadGroups) const
		{
			Dispatch(numThreadGroups, 1, 1);
		}

		template <typename DerivedClass>
		void ComputeCapableCommandGenerator<DerivedClass>::Dispatch2D(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY) const
		{
			Dispatch(numThreadGroupsX, numThreadGroupsY, 1);
		}

		template <typename DerivedClass>
		void ComputeCapableCommandGenerator<DerivedClass>::Dispatch3D(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY, const std::uint32_t numThreadGroupsZ) const
		{
			Dispatch(numThreadGroupsX, numThreadGroupsY, numThreadGroupsZ);
		}

		template <typename DerivedClass>
		template <DXGI_FORMAT Format, D3D12_UAV_DIMENSION ViewDimension>
		void ComputeCapableCommandGenerator<DerivedClass>::ClearUnorderedAccessView(const UnorderedAccessView<Format, ViewDimension>& uavToClear) const
		{
			static constexpr std::array<float, 4> DEFAULT_CLEAR_VALUE{ 0.0f, 0.0f, 0.0f, 0.0f };
			static constexpr D3D12_DESCRIPTOR_HEAP_DESC NON_SHADER_VISIBLE_HEAP_DESC{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = 1,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				.NodeMask = 0
			};

			// Create the non-shader-visible descriptor heap. The 
			// ID3D12GraphicsCommandList::ClearUnorderedAccessView*() functions require the UAV to be in both
			// a shader-visible descriptor heap and a non-shader-visible descriptor heap.
			Microsoft::WRL::ComPtr<Brawler::D3D12DescriptorHeap> stagingHeap{};
			Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateDescriptorHeap(&NON_SHADER_VISIBLE_HEAP_DESC, IID_PPV_ARGS(&stagingHeap)));

			const CD3DX12_CPU_DESCRIPTOR_HANDLE hNonShaderVisibleDescriptor{ stagingHeap->GetCPUDescriptorHandleForHeapStart() };
			Brawler::D3D12Resource& resourceToClear{ uavToClear.GetD3D12Resource() };

			// Write the UAV into the shader-visible descriptor heap.
			{
				Brawler::OptionalRef<Brawler::D3D12Resource> counterResource{ uavToClear.GetUAVCounterResource() };
				const D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{ uavToClear.CreateUAVDescription() };

				if (counterResource.HasValue()) [[unlikely]]
				{
					Util::Engine::GetD3D12Device().CreateUnorderedAccessView(
						&resourceToClear,
						&(*counterResource),
						&uavDesc,
						hNonShaderVisibleDescriptor
					);
				}
				else [[likely]]
				{
					Util::Engine::GetD3D12Device().CreateUnorderedAccessView(
						&resourceToClear,
						nullptr,
						&uavDesc,
						hNonShaderVisibleDescriptor
					);
				}
			}

			// Reserve a spot in the per-frame segment of the GPUResourceDescriptorHeap for this UAV.
			const DescriptorHandleInfo perFrameReservationHandleInfo{ Util::Engine::GetGPUResourceDescriptorHeap().CreatePerFrameDescriptorHeapReservation(1) };

			// Copy the UAV from the non-shader-visible descriptor heap to the GPUResourceDescriptorHeap.
			Util::Engine::GetD3D12Device().CopyDescriptorsSimple(
				1,
				perFrameReservationHandleInfo.HCPUDescriptor,
				hNonShaderVisibleDescriptor,
				D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);

			// Buffers do not have optimized clear values. So, if this UAV is for a buffer, then we do
			// not bother checking for it and always use DEFAULT_CLEAR_VALUE.
			//
			// We can check if this UAV is for a buffer by comparing ViewDimension to D3D12_UAV_DIMENSION_BUFFER.
			// The benefit of doing this over checking the underlying resource's Brawler::D3D12_RESOURCE_DESC
			// is that we can do this check at compile time.
			if constexpr (ViewDimension == D3D12_UAV_DIMENSION_BUFFER)
			{
				GetDerivedClassCommandList().ClearUnorderedAccessViewFloat(
					perFrameReservationHandleInfo.HGPUDescriptor,
					hNonShaderVisibleDescriptor,
					&resourceToClear,
					reinterpret_cast<const FLOAT*>(DEFAULT_CLEAR_VALUE.data()),
					0,
					nullptr
				);
			}
			else
			{
				// For all other types of resources, we should use its optimized clear value in order to clear
				// the resource, if it has one.
				const std::optional<D3D12_CLEAR_VALUE> optimizedClearValue{ uavToClear.GetGPUResource().GetOptimizedClearValue() };

				if (optimizedClearValue.has_value()) [[unlikely]]
				{
					GetDerivedClassCommandList().ClearUnorderedAccessViewFloat(
						perFrameReservationHandleInfo.HGPUDescriptor,
						hNonShaderVisibleDescriptor,
						&resourceToClear,
						optimizedClearValue->Color,
						0,
						nullptr
					);
				}
				else [[likely]]
				{
					GetDerivedClassCommandList().ClearUnorderedAccessViewFloat(
						perFrameReservationHandleInfo.HGPUDescriptor,
						hNonShaderVisibleDescriptor,
						&resourceToClear,
						reinterpret_cast<const FLOAT*>(DEFAULT_CLEAR_VALUE.data()),
						0,
						nullptr
					);
				}
			}
		}

		template <typename DerivedClass>
		Brawler::D3D12GraphicsCommandList& ComputeCapableCommandGenerator<DerivedClass>::GetDerivedClassCommandList() const
		{
			const DerivedClass* const derivedClassPtr = static_cast<const DerivedClass*>(this);
			
			if constexpr (std::derived_from<DerivedClass, GPUCommandContext<GPUCommandQueueType::DIRECT>>)
				return static_cast<const GPUCommandContext<GPUCommandQueueType::DIRECT>*>(derivedClassPtr)->GetCommandList();

			else if constexpr (std::derived_from<DerivedClass, GPUCommandContext<GPUCommandQueueType::COMPUTE>>)
				return static_cast<const GPUCommandContext<GPUCommandQueueType::COMPUTE>*>(derivedClassPtr)->GetCommandList();
		}
	}
}
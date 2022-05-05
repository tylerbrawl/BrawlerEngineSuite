module;
#include <limits>
#include <cassert>
#include <optional>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.D3D12.I_BufferSubAllocation;
import Util.HLSL;
import Util.Math;
import Util.Reflection;
import Brawler.D3D12.RootDescriptors;
import Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.UAVCounterSubAllocation;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.GPUResourceViews;

namespace Brawler
{
	namespace D3D12
	{
		template <typename T, std::size_t FieldIndex>
		consteval bool IsStructureTightlyPackedIMPL(std::size_t& currTotalFieldSize)
		{
			currTotalFieldSize += sizeof(Util::Reflection::FieldType<T, FieldIndex>);

			if constexpr (FieldIndex == (Util::Reflection::GetFieldCount<T>() - 1))
			{
				// After going through all of the fields, if the combined sizes of all of the individual
				// fields is the same as that of the type T itself, then we know that T is tightly
				// packed.

				return (currTotalFieldSize == sizeof(T));
			}
			else
				return IsStructureTightlyPackedIMPL<T, (FieldIndex + 1)>(currTotalFieldSize);
		}

		template <typename T>
			requires Util::Reflection::IsReflectable<T>
		consteval bool IsStructureTightlyPacked()
		{
			std::size_t currTotalFieldSize = 0;
			return IsStructureTightlyPackedIMPL<T, 0>(currTotalFieldSize);
		}
		
		// Strictly speaking, we don't actually need to ensure alignment for StructuredBuffers
		// matches that of ConstantBuffers in HLSL. However, we do that here for two reasons:
		//
		//   1. NVIDIA suggests aligning StructuredBuffer data to a 16-byte stride for performance.
		//
		//   2. It becomes easier to use StructuredBuffer data formats for ConstantBuffer data and
		//      vice versa.
		//
		// However, StructuredBuffers are, by definition, tightly packed, so we need to make sure
		// of that.

		template <typename T>
		concept HLSLStructuredBufferCompatible = Util::HLSL::IsHLSLConstantBufferAligned<T>() && IsStructureTightlyPacked<T>();

		static constexpr std::size_t DYNAMIC_BUFFER_SIZE = std::numeric_limits<std::size_t>::max();

		template <typename T, std::size_t NumElements>
		class SizeContainer
		{
		public:
			SizeContainer() = default;
			virtual ~SizeContainer() = default;

			consteval std::size_t GetCalculatedSize() const
			{
				return (sizeof(T) * NumElements);
			}

			consteval std::size_t GetElementCount() const
			{
				return NumElements;
			}
		};

		template <typename T>
		class SizeContainer<T, DYNAMIC_BUFFER_SIZE>
		{
		public:
			explicit SizeContainer(const std::size_t desiredSizeInElements) :
				mNumElements(desiredSizeInElements)
			{}

			virtual ~SizeContainer() = default;

			std::size_t GetCalculatedSize() const
			{
				return (sizeof(T) * mNumElements);
			}

			std::size_t GetElementCount() const
			{
				return mNumElements;
			}

		private:
			std::size_t mNumElements;
		};

		template <typename T>
		concept D3D12ConstantBufferDataPlacementAligned = (sizeof(T) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0);
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <typename T, std::size_t NumElements = DYNAMIC_BUFFER_SIZE>
			requires HLSLStructuredBufferCompatible<T>
		class StructuredBufferSubAllocation final : public I_BufferSubAllocation, private SizeContainer<T, NumElements>
		{
		public:
			struct CBVCreationInfo
			{
				DescriptorTableBuilder& TableBuilder;
				std::uint32_t DescriptorTableIndex;
				std::uint32_t ElementIndex;
			};

			struct ElementRange
			{
				std::size_t FirstElement;
				std::size_t NumElements;
			};

		private:
			template <std::uint32_t DummyParam>
			struct GeneralDescriptorCreationInfo
			{
				DescriptorTableBuilder& TableBuilder;
				std::uint32_t DescriptorTableIndex;

				/// <summary>
				/// The range of elements which is to be used. If this std::optional instance is left empty,
				/// then the entire StructuredBuffer is visible to the range. Otherwise, you can specify
				/// what is essentially a std::span of elements which will be visible through the view.
				/// 
				/// Make sure that indices in shaders take any offsets which you add into account!
				/// </summary>
				std::optional<ElementRange> ViewedElementsRange;
			};

		public:
			using SRVCreationInfo = GeneralDescriptorCreationInfo<0>;
			using UAVCreationInfo = GeneralDescriptorCreationInfo<1>;

		public:
			StructuredBufferSubAllocation() requires (NumElements != DYNAMIC_BUFFER_SIZE) = default;
			explicit StructuredBufferSubAllocation(const std::size_t numElements = 1) requires (NumElements == DYNAMIC_BUFFER_SIZE);

			std::size_t GetSubAllocationSize() const override;
			std::size_t GetRequiredDataPlacementAlignment() const override;

			/// <summary>
			/// Writes the data elements in srcDataSpan into the StructuredBuffer on the CPU
			/// timeline. The data is inserted startElementIndex data elements away from the start
			/// of the StructuredBuffer sub-allocation.
			/// 
			/// One might wonder why such an inconvenient API is exposed. Transferring data over
			/// the PCI-e lane is a slow process, so we want to transfer as much data at once as
			/// possible, rather than performing a large amount of small transfers. To that end,
			/// API users are encouraged to upload as much data as possible in one call to this
			/// function.
			/// 
			/// *NOTE*: This function will assert in Debug builds if the associated
			/// BufferResource was not created in a D3D12_HEAP_TYPE_UPLOAD heap!
			/// </summary>
			/// <param name="startElementIndex">
			/// - The offset, in data elements, from the start of the StructuredBuffer
			///   sub-allocation at which the data will be written to the GPU.
			/// </param>
			/// <param name="srcDataSpan">
			/// - A std::span which contains the data which will be written out to the
			///   BufferResource.
			/// </param>
			void WriteStructuredBufferData(const std::uint32_t startElementIndex, const std::span<const T> srcDataSpan) const;

			/// <summary>
			/// Reads the data currently stored within the StructuredBuffer and copies it to the
			/// data elements in destDataSpan on the CPU timeline. The data is read startElementIndex
			/// data elements away from the start of the StructuredBuffer sub-allocation.
			/// 
			/// One might wonder why such an inconvenient API is exposed. Transferring data over
			/// the PCI-e lane is a slow process, so we want to transfer as much data at once as
			/// possible, rather than performing a large amount of small transfers. To that end,
			/// API users are encouraged to read back as much data as possible in one call to this
			/// function.
			/// 
			/// *NOTE*: This function will assert in Debug builds if the associated
			/// BufferResource was not created in a D3D12_HEAP_TYPE_READBACK heap!
			/// </summary>
			/// <param name="startElementIndex">
			/// - The offset, in data elements, from the start of the StructuredBuffer
			///   sub-allocation at which data will be read from the GPU.
			/// </param>
			/// <param name="destDataSpan">
			/// - A std::span which contains the data which will be written out to the
			///   BufferResource. The size of this std::span will determine how many
			///   data elements will be read.
			/// </param>
			void ReadStructuredBufferData(const std::uint32_t startElementIndex, const std::span<T> destDataSpan) const;

			void SetUAVCounter(UAVCounterSubAllocation&& uavCounter);
			UAVCounterSubAllocation& GetUAVCounter() const;

			RootConstantBufferView CreateRootConstantBufferView(const std::uint32_t elementIndex) const requires D3D12ConstantBufferDataPlacementAligned<T>;
			void CreateConstantBufferViewForDescriptorTable(const CBVCreationInfo& creationInfo) const requires D3D12ConstantBufferDataPlacementAligned<T>;

			RootShaderResourceView CreateRootShaderResourceView() const;
			StructuredBufferShaderResourceView CreateShaderResourceViewForDescriptorTable() const;
			StructuredBufferShaderResourceView CreateShaderResourceViewForDescriptorTable(const ElementRange& viewedElementsRange) const;

			RootUnorderedAccessView CreateRootUnorderedAccessView() const;
			StructuredBufferUnorderedAccessView CreateUnorderedAccessViewForDescriptorTable() const;
			StructuredBufferUnorderedAccessView CreateUnorderedAccessViewForDescriptorTable(const ElementRange& viewedElementsRange) const;

		private:
			/// <summary>
			/// Given an ElementRange defining the range of elements relative to the start of this sub-allocation,
			/// this function calculates and returns the ElementRange defining the range of elements relative to
			/// the start of the BufferResource in which it is placed in.
			/// 
			/// The returned ElementRange can be used to create SRV and UAV descriptions for D3D12.
			/// </summary>
			/// <param name="rangeInRelativeUnits">
			/// - The ElementRange describing elements relative to the start of this sub-allocation.
			/// </param>
			/// <returns>
			/// The function calculates and returns the ElementRange defining the range of elements relative to
			/// the start of the BufferResource in which it is placed in.
			/// </returns>
			ElementRange ConvertElementRangeToAbsoluteUnits(const ElementRange& rangeInRelativeUnits) const;

			D3D12_BUFFER_SRV CreateBufferSRVDescription(const ElementRange& rangeInRelativeUnits) const;
			D3D12_BUFFER_UAV CreateBufferUAVDescription(const ElementRange& rangeInRelativeUnits) const;

		private:
			UAVCounterSubAllocation mUAVCounter;
		};

		// NOTE: StructuredBufferSubAllocation inherits privately from SizeContainer not for the sake of any is-a relationship,
		// as one might expect from the natural object-oriented use of inheritance, but so that the size of
		// StructuredBufferSubAllocation does not increase when the number of elements is known at compile time.
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		StructuredBufferSubAllocation<T, NumElements>::StructuredBufferSubAllocation(const std::size_t numElements) requires (NumElements == DYNAMIC_BUFFER_SIZE) :
			SizeContainer<T, NumElements>(numElements),
			mUAVCounter()
		{}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		std::size_t StructuredBufferSubAllocation<T, NumElements>::GetSubAllocationSize() const
		{
			// When NumElements != DYNAMIC_BUFFER_SIZE, this should still be evaluated at compile time, since
			// the template instantiation for SizeContainer in that case has a consteval version of
			// GetCalculatedSize().

			return this->GetCalculatedSize();
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		std::size_t StructuredBufferSubAllocation<T, NumElements>::GetRequiredDataPlacementAlignment() const
		{
			// Unlike constant buffer data reads, in DirectX 12 and HLSL, structured buffer data reads do not
			// require an explicit data placement alignment. However, by using a specific alignment value, we
			// can easily calculate where in the total BufferResource this particular sub-allocation is located
			// for the sake of creating SRVs and UAVs.

			return sizeof(T);
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		void StructuredBufferSubAllocation<T, NumElements>::WriteStructuredBufferData(const std::uint32_t startElementIndex, const std::span<const T> srcDataSpan) const
		{
			assert(static_cast<std::size_t>(startElementIndex) + srcDataSpan.size() <= this->GetElementCount());
			WriteToBuffer(srcDataSpan, (sizeof(T) * startElementIndex));
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		void StructuredBufferSubAllocation<T, NumElements>::ReadStructuredBufferData(const std::uint32_t startElementIndex, const std::span<T> destDataSpan) const
		{
			assert(static_cast<std::size_t>(startElementIndex) + destDataSpan.size() <= this->GetElementCount());
			ReadFromBuffer(destDataSpan, (sizeof(T) * startElementIndex));
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		void StructuredBufferSubAllocation<T, NumElements>::SetUAVCounter(UAVCounterSubAllocation&& uavCounter)
		{
			assert(uavCounter.HasReservation() && "ERROR: An attempt was made to assign a UAV counter to a StructuredBufferSubAllocation object, but the UAVCounterSubAllocation object was never given a BufferSubAllocationReservation!");
			mUAVCounter = std::move(uavCounter);
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		UAVCounterSubAllocation& StructuredBufferSubAllocation<T, NumElements>::GetUAVCounter() const
		{
			assert(mUAVCounter.HasReservation() && "ERROR: An attempt was made to get the UAV counter of a StructuredBufferSubAllocation object, but it was never assigned one by calling StructuredBufferSubAllocation::SetUAVCounter()!");
			return mUAVCounter;
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		RootConstantBufferView StructuredBufferSubAllocation<T, NumElements>::CreateRootConstantBufferView(const std::uint32_t elementIndex) const requires D3D12ConstantBufferDataPlacementAligned<T>
		{
			assert(static_cast<std::size_t>(elementIndex) < this->GetElementCount());

			return RootConstantBufferView{ GetGPUVirtualAddress() + (sizeof(T) * elementIndex) };
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		void StructuredBufferSubAllocation<T, NumElements>::CreateConstantBufferViewForDescriptorTable(const CBVCreationInfo& creationInfo) const requires D3D12ConstantBufferDataPlacementAligned<T>
		{
			assert(static_cast<std::size_t>(creationInfo.ElementIndex) < this->GetElementCount());

			creationInfo.TableBuilder.CreateConstantBufferView(creationInfo.ElementIndex, D3D12_CONSTANT_BUFFER_VIEW_DESC{
				.BufferLocation = GetGPUVirtualAddress() + (sizeof(T) * creationInfo.ElementIndex),
				.SizeInBytes = sizeof(T)
			});
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		RootShaderResourceView StructuredBufferSubAllocation<T, NumElements>::CreateRootShaderResourceView() const
		{
			return RootShaderResourceView{ *this };
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		StructuredBufferShaderResourceView StructuredBufferSubAllocation<T, NumElements>::CreateShaderResourceViewForDescriptorTable() const
		{
			return StructuredBufferShaderResourceView{ GetBufferResource(), CreateBufferSRVDescription(ElementRange{
				.FirstElement = 0,
				.NumElements = this->GetElementCount()
			}) };
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		StructuredBufferShaderResourceView StructuredBufferSubAllocation<T, NumElements>::CreateShaderResourceViewForDescriptorTable(const ElementRange& viewedElementsRange) const
		{
			assert(viewedElementsRange.FirstElement + viewedElementsRange.NumElements <= this->GetElementCount() && "ERROR: An invalid ElementRange was provided to StructuredBufferSubAllocation::CreateShaderResourceViewForDescriptorTable()!");
			return StructuredBufferShaderResourceView{ GetBufferResource(), CreateBufferSRVDescription(viewedElementsRange) };
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		RootUnorderedAccessView StructuredBufferSubAllocation<T, NumElements>::CreateRootUnorderedAccessView() const
		{
			return RootUnorderedAccessView{ *this };
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		StructuredBufferUnorderedAccessView StructuredBufferSubAllocation<T, NumElements>::CreateUnorderedAccessViewForDescriptorTable() const
		{
			D3D12_BUFFER_UAV viewDesc{ CreateBufferUAVDescription(ElementRange{
				.FirstElement = 0,
				.NumElements = this->GetElementCount()
			}) };

			StructuredBufferUnorderedAccessView bufferUAV{ GetBufferResource(), std::move(viewDesc)};

			if (mUAVCounter.HasReservation())
				bufferUAV.SetUAVCounter(mUAVCounter);

			return bufferUAV;
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		StructuredBufferUnorderedAccessView StructuredBufferSubAllocation<T, NumElements>::CreateUnorderedAccessViewForDescriptorTable(const ElementRange& viewedElementsRange) const
		{
			StructuredBufferUnorderedAccessView bufferUAV{ GetBufferResource(), CreateBufferUAVDescription(viewedElementsRange) };

			if (mUAVCounter.HasReservation())
				bufferUAV.SetUAVCounter(mUAVCounter);

			return bufferUAV;
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		StructuredBufferSubAllocation<T, NumElements>::ElementRange StructuredBufferSubAllocation<T, NumElements>::ConvertElementRangeToAbsoluteUnits(const ElementRange& rangeInRelativeUnits) const
		{
			return ElementRange{
				.FirstElement = (GetOffsetFromBufferStart() / GetRequiredDataPlacementAlignment()) + rangeInRelativeUnits.FirstElement,
				.NumElements = rangeInRelativeUnits.NumElements
			};
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		D3D12_BUFFER_SRV StructuredBufferSubAllocation<T, NumElements>::CreateBufferSRVDescription(const ElementRange& rangeInRelativeUnits) const
		{
			const ElementRange absoluteViewedElementsRange{ ConvertElementRangeToAbsoluteUnits(rangeInRelativeUnits) };

			return D3D12_BUFFER_SRV{
				.FirstElement = absoluteViewedElementsRange.FirstElement,
				.NumElements = static_cast<std::uint32_t>(absoluteViewedElementsRange.NumElements),
				.StructureByteStride = sizeof(T),
				.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_NONE
			};
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		D3D12_BUFFER_UAV StructuredBufferSubAllocation<T, NumElements>::CreateBufferUAVDescription(const ElementRange& rangeInRelativeUnits) const
		{
			const ElementRange absoluteViewedElementsRange{ ConvertElementRangeToAbsoluteUnits(rangeInRelativeUnits) };

			return D3D12_BUFFER_UAV{
				.FirstElement = absoluteViewedElementsRange.FirstElement,
				.NumElements = static_cast<std::uint32_t>(absoluteViewedElementsRange.NumElements),
				.StructureByteStride = sizeof(T),

				// Even if we specify nullptr for the UAV counter in ID3D12Device::CreateUnorderedAccessView(), we still
				// need CounterOffsetInBytes to be 0, as the D3D12 API requires it, for some reason.
				.CounterOffsetInBytes = (mUAVCounter.HasReservation() ? mUAVCounter.GetOffsetFromBufferStart() : 0),

				.Flags = 0
			};
		}
	}
}
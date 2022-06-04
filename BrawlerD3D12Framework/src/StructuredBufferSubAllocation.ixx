module;
#include <limits>
#include <cassert>
#include <optional>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.StructuredBufferSubAllocation;
import :StructuredBufferViewGenerator;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.I_BufferSnapshot;
import Util.HLSL;
import Util.Math;
import Util.Reflection;
import Brawler.D3D12.RootDescriptors;
import Brawler.D3D12.UAVCounterSubAllocation;
import Brawler.D3D12.BufferResource;
import Brawler.OptionalRef;

export import :StructuredBufferElementRange;

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
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		class StructuredBufferSubAllocation;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
			requires HLSLStructuredBufferCompatible<T>
		class StructuredBufferSnapshot final : public I_BufferSnapshot, public StructuredBufferViewGenerator<StructuredBufferSnapshot<T>, T>
		{
		public:
			template <std::size_t NumElements>
			explicit StructuredBufferSnapshot(const StructuredBufferSubAllocation<T, NumElements>& sbSubAllocation);

			StructuredBufferSnapshot(const StructuredBufferSnapshot& rhs) = default;
			StructuredBufferSnapshot& operator=(const StructuredBufferSnapshot& rhs) = default;

			StructuredBufferSnapshot(StructuredBufferSnapshot&& rhs) noexcept = default;
			StructuredBufferSnapshot& operator=(StructuredBufferSnapshot&& rhs) noexcept = default;

			Brawler::OptionalRef<const UAVCounterSnapshot> GetUAVCounter() const;

			std::size_t GetElementCount() const;

		private:
			std::optional<UAVCounterSnapshot> mUAVCounterSnapshot;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace D3D12
	{
		template <typename T, std::size_t NumElements = DYNAMIC_BUFFER_SIZE>
			requires HLSLStructuredBufferCompatible<T>
		class StructuredBufferSubAllocation final : public I_BufferSubAllocation, public StructuredBufferViewGenerator<StructuredBufferSubAllocation<T, NumElements>, T>, private SizeContainer<T, NumElements>
		{
		private:
			struct UAVCounterContainer
			{
				UAVCounterSubAllocation SubAllocation;
				UAVCounterSnapshot Snapshot;
			};

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
			Brawler::OptionalRef<const UAVCounterSnapshot> GetUAVCounter() const;

			std::size_t GetElementCount() const;

		private:
			std::optional<UAVCounterContainer> mUAVCounterContainer;
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
		template <typename T>
			requires HLSLStructuredBufferCompatible<T>
		template <std::size_t NumElements>
		StructuredBufferSnapshot<T>::StructuredBufferSnapshot(const StructuredBufferSubAllocation<T, NumElements>& sbSubAllocation) :
			I_BufferSnapshot(sbSubAllocation),
			mUAVCounterSnapshot()
		{
			const Brawler::OptionalRef<const UAVCounterSnapshot> uavCounterSnapshot{ sbSubAllocation.GetUAVCounter() };

			if (uavCounterSnapshot.HasValue()) [[unlikely]]
				mUAVCounterSnapshot = *uavCounterSnapshot;
		}

		template <typename T>
			requires HLSLStructuredBufferCompatible<T>
		Brawler::OptionalRef<const UAVCounterSnapshot> StructuredBufferSnapshot<T>::GetUAVCounter() const
		{
			if (mUAVCounterSnapshot.has_value()) [[unlikely]]
				return Brawler::OptionalRef<const UAVCounterSnapshot>{ *mUAVCounterSnapshot };

			return Brawler::OptionalRef<const UAVCounterSnapshot>{};
		}

		template <typename T>
			requires HLSLStructuredBufferCompatible<T>
		std::size_t StructuredBufferSnapshot<T>::GetElementCount() const
		{
			return (GetSubAllocationSize() / sizeof(T));
		}
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
			I_BufferSubAllocation(),
			StructuredBufferViewGenerator<StructuredBufferSubAllocation<T, NumElements>, T>(),
			SizeContainer<T, NumElements>(numElements),
			mUAVCounterContainer()
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
			mUAVCounterContainer = UAVCounterContainer{
				.SubAllocation{std::move(uavCounter)},
				.Snapshot{ mUAVCounterContainer.SubAllocation }
			};
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		Brawler::OptionalRef<const UAVCounterSnapshot> StructuredBufferSubAllocation<T, NumElements>::GetUAVCounter() const
		{
			if (!mUAVCounterContainer.has_value()) [[likely]]
				return Brawler::OptionalRef<const UAVCounterSnapshot>{};

			return Brawler::OptionalRef<const UAVCounterSnapshot>{ mUAVCounterContainer->Snapshot };
		}

		template <typename T, std::size_t NumElements>
			requires HLSLStructuredBufferCompatible<T>
		std::size_t StructuredBufferSubAllocation<T, NumElements>::GetElementCount() const
		{
			return SizeContainer<T, NumElements>::GetElementCount();
		}
	}
}
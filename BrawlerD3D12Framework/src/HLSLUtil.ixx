module;
#include <array>

export module Util.HLSL;
import Util.Reflection;

namespace Util
{
	namespace HLSL
	{
		template <typename T>
		struct ArrayInfo
		{
			static constexpr bool IS_ARRAY = false;
		};

		template <typename ElementType_, std::size_t NumElements_>
		struct ArrayInfo<std::array<ElementType_, NumElements_>>
		{
			static constexpr bool IS_ARRAY = true;

			using ElementType = ElementType_;
			static constexpr std::size_t ELEMENT_COUNT = NumElements_;
		};

		template <typename T>
		struct HLSLFieldSolver
		{
		private:
			static constexpr std::size_t CBUFFER_PACKING_BOUNDARY = 16;

		public:
			template <std::size_t FieldIndex>
			consteval bool CheckField(std::size_t& currSize) const
			{
				using CurrFieldType = ::Util::Reflection::FieldType<T, FieldIndex>;

				// Pointers and references have no place in HLSL constant buffers, so we explicitly
				// disable them here. It is worth noting, however, that the reflection will cause
				// C-style arrays to decay back to pointers; this means that C-style arrays will
				// also fail this check.
				//
				// To work around this, we add explicit support for std::array below. (Besides,
				// std::arrays are infinitely superior to C-style arrays because they do not decay
				// into pointers and retain their size.)
				if constexpr (std::is_pointer_v<CurrFieldType>)
					return false;

				if constexpr (ArrayInfo<CurrFieldType>::IS_ARRAY)
				{
					// Arrays in HLSL are packed differently. They *MUST* start on a 16-byte boundary.
					// In addition, each element in the array is stored in a 16-byte vector. So, to
					// ensure that memory copies from CPU to GPU work correctly, each element within
					// an array should be 16 bytes large.

					// Ensure that the array starts on a 16-byte boundary.
					if (currSize % CBUFFER_PACKING_BOUNDARY != 0)
						return false;

					// Ensure that the elements in the array have a size which is a multiple of 16
					// bytes.
					const std::size_t elementSize = sizeof(typename ArrayInfo<CurrFieldType>::ElementType);
					if (elementSize % CBUFFER_PACKING_BOUNDARY != 0)
						return false;

					currSize += (elementSize * ArrayInfo<CurrFieldType>::ELEMENT_COUNT);
				}
				else
				{
					// For all other data, HLSL will attempt to pack the values into 16-byte vectors,
					// with the restriction that no variable can straddle one of these vectors.
					const std::size_t currRemainder = currSize % CBUFFER_PACKING_BOUNDARY;

					// If the next field would cause us to move into a second vector, then we reject the
					// layout.
					if (currRemainder != 0 && currRemainder + sizeof(CurrFieldType) > CBUFFER_PACKING_BOUNDARY)
						return false;

					currSize += sizeof(CurrFieldType);
				}

				// Evaluate the next field.
				return CheckField<FieldIndex + 1>(currSize);
			}

			template <>
			consteval bool CheckField<::Util::Reflection::GetFieldCount<T>()>(std::size_t& currSize) const
			{
				return (currSize % CBUFFER_PACKING_BOUNDARY == 0);
			}
		};
	}
}

export namespace Util
{
	namespace HLSL
	{
		template <typename T>
			requires Util::Reflection::IsReflectable<T>
		consteval bool IsHLSLConstantBufferAligned()
		{
			std::size_t currSize = 0;

			const HLSLFieldSolver<T> fieldSolver{};
			return fieldSolver.CheckField<0>(currSize);
		}
	}
}
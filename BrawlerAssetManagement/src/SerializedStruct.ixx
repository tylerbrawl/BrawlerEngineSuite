module;
#include <cstddef>
#include <type_traits>
#include <variant>
#include <array>

export module Brawler.SerializedStruct;
import Util.Reflection;

namespace Brawler
{
	template <typename T, std::size_t CurrFieldIndex>
		requires Util::Reflection::IsReflectable<T>
	consteval bool IsSerializableIMPL();

	template <typename T>
	struct ArraySolver
	{
		static constexpr bool IS_ARRAY = false;
	};

	template <typename DataType, std::size_t Size>
	struct ArraySolver<std::array<DataType, Size>>
	{
		static constexpr bool IS_ARRAY = true;
	};

	template <typename FieldType>
	consteval bool IsFieldSerializable()
	{
		// Don't allow pointers or references; these cannot be serialized meaningfully.
		if constexpr (std::is_pointer_v<FieldType> || std::is_reference_v<FieldType>)
			return false;

		// Do, however, allow arithmetic primitives.
		if constexpr (std::is_arithmetic_v<FieldType>)
			return true;

		// Add explicit support for std::array.
		constexpr bool IS_ARRAY = ArraySolver<FieldType>::IS_ARRAY;

		if constexpr (IS_ARRAY)
			return true;

		// If we cannot verify that the type is serializable via reflection, then
		// do not allow its serialization.
		if constexpr (Util::Reflection::IsReflectable<FieldType>)
		{
			constexpr std::size_t FIELD_COUNT = Util::Reflection::GetFieldCount<FieldType>();

			if constexpr (FIELD_COUNT > 1)
				return IsSerializableIMPL<FieldType, 0>();
			else
				return true;
		}
		else
			return false;
	}

	template <typename T, std::size_t CurrFieldIndex>
		requires Util::Reflection::IsReflectable<T>
	consteval bool IsSerializableIMPL()
	{
		if constexpr (CurrFieldIndex == Util::Reflection::GetFieldCount<T>())
			return true;
		else
		{
			using CurrField = Util::Reflection::FieldType<T, CurrFieldIndex>;

			if (!IsFieldSerializable<CurrField>())
				return false;

			return IsSerializableIMPL<T, (CurrFieldIndex + 1)>();
		}
	}

	template <typename T>
	concept IsSerializable = Util::Reflection::IsReflectable<T> && IsSerializableIMPL<T, 0>();
}

namespace Brawler
{
	// I don't know of a better way to implement this...

#pragma pack(push)
#pragma pack(1)
	template <typename T, std::size_t FieldCount>
	struct SerializedStructIMPL
	{
		static_assert(sizeof(T) != sizeof(T), "ERROR: It looks like you need to add more explicit template specializations for Brawler::SerializedStructIMPL. (See SerializedStruct.ixx.) Have fun!");
	};

	template <typename T>
		requires IsSerializable<T>
	struct SerializedStructIMPL<T, 1>
	{
		Util::Reflection::FieldType<T, 0> Field0;
	};

	template <typename T>
		requires IsSerializable<T>
	struct SerializedStructIMPL<T, 2>
	{
		Util::Reflection::FieldType<T, 0> Field0;
		Util::Reflection::FieldType<T, 1> Field1;
	};

	template <typename T>
		requires IsSerializable<T>
	struct SerializedStructIMPL<T, 3>
	{
		Util::Reflection::FieldType<T, 0> Field0;
		Util::Reflection::FieldType<T, 1> Field1;
		Util::Reflection::FieldType<T, 2> Field2;
	};

	template <typename T>
		requires IsSerializable<T>
	struct SerializedStructIMPL<T, 4>
	{
		Util::Reflection::FieldType<T, 0> Field0;
		Util::Reflection::FieldType<T, 1> Field1;
		Util::Reflection::FieldType<T, 2> Field2;
		Util::Reflection::FieldType<T, 3> Field3;
	};

	template <typename T>
		requires IsSerializable<T>
	struct SerializedStructIMPL<T, 5>
	{
		Util::Reflection::FieldType<T, 0> Field0;
		Util::Reflection::FieldType<T, 1> Field1;
		Util::Reflection::FieldType<T, 2> Field2;
		Util::Reflection::FieldType<T, 3> Field3;
		Util::Reflection::FieldType<T, 4> Field4;
	};

	template <typename T>
		requires IsSerializable<T>
	struct SerializedStructIMPL<T, 6>
	{
		Util::Reflection::FieldType<T, 0> Field0;
		Util::Reflection::FieldType<T, 1> Field1;
		Util::Reflection::FieldType<T, 2> Field2;
		Util::Reflection::FieldType<T, 3> Field3;
		Util::Reflection::FieldType<T, 4> Field4;
		Util::Reflection::FieldType<T, 5> Field5;
	};

	template <typename T>
		requires IsSerializable<T>
	struct SerializedStructIMPL<T, 7>
	{
		Util::Reflection::FieldType<T, 0> Field0;
		Util::Reflection::FieldType<T, 1> Field1;
		Util::Reflection::FieldType<T, 2> Field2;
		Util::Reflection::FieldType<T, 3> Field3;
		Util::Reflection::FieldType<T, 4> Field4;
		Util::Reflection::FieldType<T, 5> Field5;
		Util::Reflection::FieldType<T, 6> Field6;
	};

	template <typename T>
		requires IsSerializable<T>
	struct SerializedStructIMPL<T, 8>
	{
		Util::Reflection::FieldType<T, 0> Field0;
		Util::Reflection::FieldType<T, 1> Field1;
		Util::Reflection::FieldType<T, 2> Field2;
		Util::Reflection::FieldType<T, 3> Field3;
		Util::Reflection::FieldType<T, 4> Field4;
		Util::Reflection::FieldType<T, 5> Field5;
		Util::Reflection::FieldType<T, 6> Field6;
		Util::Reflection::FieldType<T, 7> Field7;
	};

	template <typename T>
		requires IsSerializable<T>
	struct SerializedStructIMPL<T, 9>
	{
		Util::Reflection::FieldType<T, 0> Field0;
		Util::Reflection::FieldType<T, 1> Field1;
		Util::Reflection::FieldType<T, 2> Field2;
		Util::Reflection::FieldType<T, 3> Field3;
		Util::Reflection::FieldType<T, 4> Field4;
		Util::Reflection::FieldType<T, 5> Field5;
		Util::Reflection::FieldType<T, 6> Field6;
		Util::Reflection::FieldType<T, 7> Field7;
		Util::Reflection::FieldType<T, 8> Field8;
	};

	template <typename T>
		requires IsSerializable<T>
	struct SerializedStructIMPL<T, 10>
	{
		Util::Reflection::FieldType<T, 0> Field0;
		Util::Reflection::FieldType<T, 1> Field1;
		Util::Reflection::FieldType<T, 2> Field2;
		Util::Reflection::FieldType<T, 3> Field3;
		Util::Reflection::FieldType<T, 4> Field4;
		Util::Reflection::FieldType<T, 5> Field5;
		Util::Reflection::FieldType<T, 6> Field6;
		Util::Reflection::FieldType<T, 7> Field7;
		Util::Reflection::FieldType<T, 8> Field8;
		Util::Reflection::FieldType<T, 9> Field9;
	};
#pragma pack(pop)
}

namespace Brawler
{
	template <typename T>
	struct FieldCountSolver
	{
		// Originally, we had the following type alias for SerializedStruct:
		//
		// using SerializedStruct = SerializedStructIMPL<T, Util::Reflection::GetFieldCount<T>()>;
		//
		// However, this was causing MSVC crashes/internal compiler errors, so we use this extra
		// level of indirection. Somehow, this works while the other type alias does not. Go figure.
		static constexpr std::size_t FIELD_COUNT = Util::Reflection::GetFieldCount<T>();
	};
}

export namespace Brawler
{
	/// <summary>
	/// Given a reflectable type T, SerializedStruct&lt;T&gt; is a type which describes the structure
	/// T, but without any alignment padding in between members. A similar effect can be achieved
	/// by using "#pragma pack(1)" before defining the type T, but using this structure allows one
	/// to automatically create the equivalent type.
	/// 
	/// Packing fields close together in memory like this is great for cross-platform serialization,
	/// but is generally terrible for performance.
	/// </summary>
	/// <typeparam name="T">
	/// - The type which is to be serialized. It cannot contain any pointers or references, since
	///   these cannot be serialized. (C-style arrays will decay to pointers as a result of reflection,
	///   but there is a hardcoded exception to allow for std::array.) All of the fields of T are
	///   also recursively checked to see if they can be serialized.
	/// </typeparam>
	template <typename T>
		requires IsSerializable<T>
	using SerializedStruct = SerializedStructIMPL<T, FieldCountSolver<T>::FIELD_COUNT>;

	/// <summary>
	/// This concept describes whether the type T is "inherently serializable;" that is, that
	/// T is not only reflectable (as in Util::Reflection::IsReflectable) but also has the same
	/// memory layout and size of SerializedStruct&lt;T&gt;.
	/// 
	/// A type which is not inherently serializable may still very well be serializable. However,
	/// the performance of serializing and deserializing the data will be greatly diminished in
	/// order to account for alignment differences. If this performance penalty is unacceptable for
	/// your target architecture, consider adding padding to types which should be serialized so
	/// that they become inherently serializable.
	/// </summary>
	template <typename T>
	concept IsInherentlySerializable = IsSerializable<T> && (sizeof(T) == sizeof(SerializedStruct<T>));
}

namespace Brawler
{
	template <typename T, std::size_t FieldIndex>
	constexpr void SerializeField(SerializedStruct<T>& destStruct, const T& srcStruct)
	{
		if constexpr (FieldIndex != Util::Reflection::GetFieldCount<T>())
		{
			auto& destField = Util::Reflection::GetFieldReference<SerializedStruct<T>, FieldIndex>(destStruct);
			const auto& srcField = Util::Reflection::GetFieldReference<T, FieldIndex>(srcStruct);

			std::memcpy(std::addressof(destField), std::addressof(srcField), sizeof(srcField));

			SerializeField<T, (FieldIndex + 1)>(destStruct, srcStruct);
		}
	}

	template <typename T, std::size_t FieldIndex>
	constexpr void DeserializeField(T& destStruct, const SerializedStruct<T>& srcStruct)
	{
		if constexpr (FieldIndex != Util::Reflection::GetFieldCount<T>())
		{
			auto& destField = Util::Reflection::GetFieldReference<T, FieldIndex>(destStruct);
			const auto& srcField = Util::Reflection::GetFieldReference<SerializedStruct<T>, FieldIndex>(srcStruct);

			std::memcpy(std::addressof(destField), std::addressof(srcField), sizeof(srcField));

			DeserializeField<T, (FieldIndex + 1)>(destStruct, srcStruct);
		}
	}
}

export namespace Brawler
{
	template <typename T>
		requires IsSerializable<T>
	constexpr SerializedStruct<T> SerializeData(const T& data)
	{
		SerializedStruct<T> serializedData;

		// If the size of T is the same as the size of SerializedStruct<T>, then we know that we can
		// safely copy the entire structure over in one std::memcpy operation. Otherwise, we need
		// to carefully copy each individual field to ensure proper alignment.
		if constexpr (sizeof(T) == sizeof(SerializedStruct<T>))
			std::memcpy(&serializedData, std::addressof(data), sizeof(data));
		else
			SerializeField<T, 0>(serializedData, data);

		return serializedData;
	}

	template <typename T>
		requires IsSerializable<T>
	constexpr T DeserializeData(const SerializedStruct<T>& serializedData)
	{
		T data;

		// If the size of T is the same as the size of SerializedStruct<T>, then we know that we can
		// safely copy the entire structure over in one std::memcpy operation. Otherwise, we need
		// to carefully copy each individual field to ensure proper alignment.
		if constexpr (sizeof(T) == sizeof(SerializedStruct<T>))
			std::memcpy(std::addressof(data), &serializedData, sizeof(serializedData));
		else
			DeserializeField<T, 0>(data, serializedData);

		return data;
	}
}
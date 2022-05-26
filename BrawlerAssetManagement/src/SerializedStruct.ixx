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
		requires Util::Reflection::IsReflectable<T>
	concept IsSerializable = IsSerializableIMPL<T, 0>();
}

namespace Brawler
{
#pragma pack(push)
#pragma pack(1)
	template <typename T, std::size_t CurrField>
		requires IsSerializable<T> && (Util::Reflection::GetFieldCount<T>() == CurrField)
	struct SerializedStructIMPL
	{};

	template <typename T, std::size_t CurrField>
		requires IsSerializable<T> && (Util::Reflection::GetFieldCount<T>() != CurrField)
	struct SerializedStructIMPL
	{
		Util::Reflection::FieldType<T, CurrField> CurrentField;
		[[no_unique_address]] SerializedStructIMPL<T, (CurrField + 1)> NextField;
	};
#pragma pack(pop)
}

export namespace Brawler
{
	template <typename T>
		requires IsSerializable<T>
	using SerializedStruct = SerializedStructIMPL<T, 0>;
}

namespace Brawler
{
	template <typename T, std::size_t FieldIndex>
	void SerializeField(SerializedStruct<T>& destStruct, const T& srcStruct)
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
	void DeserializeField(T& destStruct, const SerializedStruct<T>& srcStruct)
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
	SerializedStruct<T> SerializeData(const T& data)
	{
		SerializedStruct<T> serializedData;

		// If the size of T is the same as the size of SerializedStruct<T>, then we know that we can
		// safely copy the entire structure over in one std::memcpy operation. Otherwise, we need
		// to carefully copy each individual field to ensure proper alignment.
		if constexpr (sizeof(T) == sizeof(SerializedStruct<T>))
			std::memcpy(std::addressof(serializedData), std::addressof(data), sizeof(data));
		else
			SerializeField<T, 0>(serializedData, data);

		return serializedData;
	}

	template <typename T>
		requires IsSerializable<T>
	T DeserializeData(const SerializedStruct<T>& serializedData)
	{
		T data;

		// If the size of T is the same as the size of SerializedStruct<T>, then we know that we can
		// safely copy the entire structure over in one std::memcpy operation. Otherwise, we need
		// to carefully copy each individual field to ensure proper alignment.
		if constexpr (sizeof(T) == sizeof(SerializedStruct<T>))
			std::memcpy(std::addressof(data), std::addressof(serializedData), sizeof(serializedData));
		else
			DeserializeField<T, 0>(data, serializedData);

		return data;
	}
}
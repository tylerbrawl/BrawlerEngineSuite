module;
#include <array>

export module Brawler.SerializedStruct:SerializedStructConcepts;
import Util.Reflection;

export namespace Brawler
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

		using ElementType = DataType;
	};

	template <typename FieldType>
	consteval bool IsFieldSerializable()
	{
		// Don't allow pointers or references; these cannot be serialized meaningfully.
		if constexpr (std::is_pointer_v<FieldType> || std::is_reference_v<FieldType>)
			return false;

		// Do, however, allow arithmetic primitives and enumeration types.
		if constexpr (std::is_arithmetic_v<FieldType> || std::is_enum_v<FieldType>)
			return true;

		// Add explicit support for std::array, but only if the type it refers
		// to is serializable.
		constexpr bool IS_ARRAY = ArraySolver<FieldType>::IS_ARRAY;

		if constexpr (IS_ARRAY)
			return IsFieldSerializable<typename ArraySolver<FieldType>::ElementType>();

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
}
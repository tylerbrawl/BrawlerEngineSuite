module;
#include <type_traits>

export module Brawler.MSVCConcepts;

export namespace Brawler
{
	namespace Concepts
	{
		/// <summary>
		/// This concept is a workaround for the MSVC bug where template enum class/scoped enum arguments
		/// result in the following error in valid C++ code:
		/// 
		/// error C2440: 'specialization': cannot convert from 'int' to 'EnumType'
		/// 
		/// where EnumType is the type of the template argument. Note that here, "int" is specified because
		/// it is the default underlying type of an enum class. If you override this underlying type, then
		/// that new type will replace "int."
		/// </summary>
		template <typename LHSType, typename RHSType>
		concept IsEnumEquivalent = (std::is_enum_v<LHSType> && !std::is_enum_v<RHSType> && std::is_same_v<std::underlying_type_t<LHSType>, RHSType>) ||
			(std::is_enum_v<RHSType> && !std::is_enum_v<LHSType> && std::is_same_v<std::underlying_type_t<RHSType>, LHSType>) ||
			(std::is_enum_v<LHSType> && std::is_enum_v<RHSType> && std::is_same_v<LHSType, RHSType>);
	}
}
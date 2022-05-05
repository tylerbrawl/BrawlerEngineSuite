module;
#include <type_traits>
#include <cstdint>

export module Brawler.IMPL.RootParamDef;
import Brawler.RootParameterEnums;
import Brawler.MSVCConcepts;

export namespace Brawler
{
	class GPUResourceHandle;
	class ResourceDescriptorTable;
}

namespace Brawler
{
	namespace IMPL
	{
		export enum class RootParamType
		{
			UINT32_CONSTANT,
			CBV,
			SRV,
			UAV,
			DESCRIPTOR_TABLE
		};

		template <typename T, T ParamType>
			requires Brawler::Concepts::IsEnumEquivalent<T, RootParamType>
		struct BoundParameterType
		{};

		template <>
		struct BoundParameterType<RootParamType, RootParamType::UINT32_CONSTANT>
		{
			using Type = const std::uint32_t;
		};

		template <>
		struct BoundParameterType<RootParamType, RootParamType::CBV>
		{
			using Type = const Brawler::GPUResourceHandle&;
		};

		template <>
		struct BoundParameterType<RootParamType, RootParamType::SRV>
		{
			using Type = const Brawler::GPUResourceHandle&;
		};

		template <>
		struct BoundParameterType<RootParamType, RootParamType::UAV>
		{
			using Type = const Brawler::GPUResourceHandle&;
		};

		template <>
		struct BoundParameterType<RootParamType, RootParamType::DESCRIPTOR_TABLE>
		{
			using Type = Brawler::ResourceDescriptorTable&;
		};

		// Root parameter index 0 is reserved for the bindless SRV descriptor table. It is a compile-time
		// error for a different root parameter to use this index. (See Brawler::IMPL::GetRootParameterIndex()
		// below to see how this is enforced.)
		static constexpr std::uint32_t BINDLESS_SRV_ROOT_PARAMETER_INDEX = 0;

		template <std::uint32_t RootParamIndex, RootParamType ParamType>
		struct RootParameterDefinition
		{
			static constexpr std::uint32_t ROOT_PARAMETER_INDEX = RootParamIndex;
			static constexpr RootParamType ROOT_PARAMETER_TYPE = ParamType;
		};

		template <typename T, T RootParamID>
			requires std::is_enum_v<T>
		struct RootParameter
		{
			using Definition = RootParameterDefinition<BINDLESS_SRV_ROOT_PARAMETER_INDEX, RootParamType::DESCRIPTOR_TABLE>;
		};

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Compute Skinning Root Parameters                                                                         //
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////

		template <>
		struct RootParameter<Brawler::RootParameters::ComputeSkinningRootParameter, Brawler::RootParameters::ComputeSkinningRootParameter::VERTEX_BUFFER_UAV>
		{
			using Definition = RootParameterDefinition<1, RootParamType::UAV>;
		};

		template <>
		struct RootParameter<Brawler::RootParameters::ComputeSkinningRootParameter, Brawler::RootParameters::ComputeSkinningRootParameter::BONE_WEIGHTS_SRV>
		{
			using Definition = RootParameterDefinition<2, RootParamType::SRV>;
		};
	}
}

export namespace Brawler
{
	namespace IMPL
	{
		template <typename T, T ParamType>
			requires Brawler::Concepts::IsEnumEquivalent<T, RootParamType>
		using RootParameterBindingType = BoundParameterType<RootParamType, static_cast<RootParamType>(ParamType)>::Type;

		template <typename T, T RootParamID>
			requires std::is_enum_v<T>
		constexpr std::uint32_t GetRootParameterIndex()
		{
			constexpr std::uint32_t rootParameterIndex = RootParameter<T, RootParamID>::Definition::ROOT_PARAMETER_INDEX;

			// Disallow root parameters from attempting to reserve the same root parameter index
			// as that which is dedicated to the bindless SRV descriptor table. 
			// 
			// This is an effective protection mechanism because there is no other way for other 
			// modules to get access to a root parameter index than to call this function.
			static_assert(rootParameterIndex != BINDLESS_SRV_ROOT_PARAMETER_INDEX, "ERROR: A root parameter was assigned to the same root parameter index as the bindless SRV descriptor table!");
			
			return rootParameterIndex;
		}

		template <typename T, T RootParamID>
			requires std::is_enum_v<T>
		constexpr RootParamType GetRootParameterType()
		{
			return RootParameter<T, RootParamID>::Definition::ROOT_PARAMETER_TYPE;
		}
	}
}
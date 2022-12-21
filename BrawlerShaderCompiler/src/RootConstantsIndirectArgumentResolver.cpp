module;
#include <string>
#include <string_view>
#include <optional>
#include <cassert>
#include <format>

module Brawler.IndirectArgumentResolvers;

namespace Brawler
{
	RootConstantsIndirectArgumentResolver::RootConstantsIndirectArgumentResolver(std::string argumentNameStr) :
		I_IndirectArgumentResolver(),
		mArgumentNameStr(std::move(argumentNameStr)),
		mTypeInfo(),
		mBindingInfo()
	{}

	void RootConstantsIndirectArgumentResolver::SetRootConstantsType(IndirectRootConstantsPreDefinedTypeInfo&& preDefinedTypeInfo)
	{
		assert(preDefinedTypeInfo.SizeInBytes > 0 && "ERROR: An attempt was made to specify a type of zero size in a call to RootConstantsIndirectArgumentResolver::SetRootConstantsType()!");
		assert(preDefinedTypeInfo.SizeInBytes % sizeof(std::uint32_t) == 0 && "ERROR: The size of the type used to set root constants via indirect arguments must be a multiple of four (4) bytes!");
		
		mTypeInfo = TypeInfo{
			.TypeIdentifierStr{ std::move(preDefinedTypeInfo.TypeIdentifierStr) },
			.TypeDefinitionStr{},
			.SizeInBytes = preDefinedTypeInfo.SizeInBytes
		};
	}

	void RootConstantsIndirectArgumentResolver::SetRootConstantsType(IndirectRootConstantsCustomTypeInfo&& customTypeInfo)
	{
		assert(customTypeInfo.SizeInBytes > 0 && "ERROR: An attempt was made to specify a type of zero size in a call to RootConstantsIndirectArgumentResolver::SetRootConstantsType()!");
		assert(customTypeInfo.SizeInBytes % sizeof(std::uint32_t) == 0 && "ERROR: The size of the type used to set root constants via indirect arguments must be a multiple of four (4) bytes!");
		
		mTypeInfo = TypeInfo{
			.TypeIdentifierStr{ std::move(customTypeInfo.TypeIdentifierStr) },
			.TypeDefinitionStr{ std::move(customTypeInfo.TypeDefinitionStr) },
			.SizeInBytes = customTypeInfo.SizeInBytes
		};
	}

	void RootConstantsIndirectArgumentResolver::SetRootParameterBinding(const IndirectRootConstantsBindingInfo bindingInfo)
	{
		mBindingInfo = bindingInfo;
	}

	std::string RootConstantsIndirectArgumentResolver::CreateIndirectArgumentDescriptionInitializerString() const
	{
		assert(HasValidRootConstantsType() && "ERROR: A RootConstantsIndirectArgumentResolver instance was never provided with a type for its root constants argument! (This can be done with RootConstantsIndirectArgumentResolver::SetRootConstantsType().)");
		assert(HasValidRootParameterBinding() && "ERROR: A RootConstantsIndirectArgumentResolver instance was never assigned to a specific root parameter destination! (This can be done with RootConstantsIndirectArgumentResolver::SetRootParameterBinding().)");

		static constexpr std::string_view INITIALIZER_FORMAT_STR{
R"(				.Type = D3D12_INDIRECT_ARGUMENT_TYPE::D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT,
				.Constant{{
					.RootParameterIndex = {},
					.DestOffsetIn32BitValues = {},
					.Num32BitValuesToSet = {}
				}})"
		};

		return std::format(
			INITIALIZER_FORMAT_STR,
			mBindingInfo->RootParameterIndex,
			mBindingInfo->OffsetIn32BitConstantsToDestination,
			(mTypeInfo.SizeInBytes / sizeof(std::uint32_t))
		);
	}

	std::string_view RootConstantsIndirectArgumentResolver::GetIndirectArgumentFieldTypeString() const
	{
		assert(HasValidRootConstantsType() && "ERROR: A RootConstantsIndirectArgumentResolver instance was never provided with a type for its root constants argument! (This can be done with RootConstantsIndirectArgumentResolver::SetRootConstantsType().)");

		return std::string_view{ *(mTypeInfo.TypeIdentifierStr) };
	}

	std::string_view RootConstantsIndirectArgumentResolver::GetIndirectArgumentFieldNameString() const
	{
		return std::string_view{ mArgumentNameStr };
	}

	std::optional<std::string_view> RootConstantsIndirectArgumentResolver::GetCustomTypeDefinitionString() const
	{
		assert(HasValidRootConstantsType() && "ERROR: A RootConstantsIndirectArgumentResolver instance was never provided with a type for its root constants argument! (This can be done with RootConstantsIndirectArgumentResolver::SetRootConstantsType().)");

		// It's possible that a pre-defined type like std::uint32_t is being used, so the custom
		// type definition may not exist.
		if (!mTypeInfo.TypeDefinitionStr.has_value())
			return {};

		return std::string_view{ *(mTypeInfo.TypeDefinitionStr) };
	}

	bool RootConstantsIndirectArgumentResolver::HasValidRootConstantsType() const
	{
		return mTypeInfo.TypeIdentifierStr.has_value();
	}

	bool RootConstantsIndirectArgumentResolver::HasValidRootParameterBinding() const
	{
		return mBindingInfo.has_value();
	}
}
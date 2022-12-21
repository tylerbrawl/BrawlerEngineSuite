module;
#include <string>
#include <string_view>
#include <optional>

export module Brawler.IndirectArgumentResolvers:RootConstantsIndirectArgumentResolver;
import :I_IndirectArgumentResolver;

export namespace Brawler
{
	struct IndirectRootConstantsPreDefinedTypeInfo
	{
		std::string TypeIdentifierStr;
		std::size_t SizeInBytes;
	};

	struct IndirectRootConstantsCustomTypeInfo
	{
		std::string TypeIdentifierStr;
		std::string TypeDefinitionStr;
		std::size_t SizeInBytes;
	};

	struct IndirectRootConstantsBindingInfo
	{
		std::uint32_t RootParameterIndex;
		std::uint32_t OffsetIn32BitConstantsToDestination;
	};

	class RootConstantsIndirectArgumentResolver final : public I_IndirectArgumentResolver
	{
	private:
		struct TypeInfo
		{
			std::optional<std::string> TypeIdentifierStr;
			std::optional<std::string> TypeDefinitionStr;
			std::size_t SizeInBytes;
		};

	public:
		explicit RootConstantsIndirectArgumentResolver(std::string argumentNameStr);

		RootConstantsIndirectArgumentResolver(const RootConstantsIndirectArgumentResolver& rhs) = delete;
		RootConstantsIndirectArgumentResolver& operator=(const RootConstantsIndirectArgumentResolver& rhs) = delete;

		RootConstantsIndirectArgumentResolver(RootConstantsIndirectArgumentResolver&& rhs) noexcept = default;
		RootConstantsIndirectArgumentResolver& operator=(RootConstantsIndirectArgumentResolver&& rhs) noexcept = default;

		void SetRootConstantsType(IndirectRootConstantsPreDefinedTypeInfo&& preDefinedTypeInfo);
		void SetRootConstantsType(IndirectRootConstantsCustomTypeInfo&& customTypeInfo);

		void SetRootParameterBinding(const IndirectRootConstantsBindingInfo bindingInfo);

		std::string CreateIndirectArgumentDescriptionInitializerString() const override;
		std::string_view GetIndirectArgumentFieldTypeString() const override;
		std::string_view GetIndirectArgumentFieldNameString() const override;
		std::optional<std::string_view> GetCustomTypeDefinitionString() const override;

	private:
		bool HasValidRootConstantsType() const;
		bool HasValidRootParameterBinding() const;

	private:
		std::string mArgumentNameStr;
		TypeInfo mTypeInfo;
		std::optional<IndirectRootConstantsBindingInfo> mBindingInfo;
	};
}
module;
#include <string>
#include <string_view>
#include <optional>

export module Brawler.IndirectArgumentResolvers:RootDescriptorIndirectArgumentResolvers;
import :I_IndirectArgumentResolver;

namespace Brawler
{
	enum class RootDescriptorID
	{
		CBV,
		SRV,
		UAV
	};
}

namespace Brawler
{
	template <RootDescriptorID DescriptorID>
	class RootDescriptorIndirectArgumentResolver final : public I_IndirectArgumentResolver
	{
	public:
		explicit RootDescriptorIndirectArgumentResolver(std::string argumentNameStr);

		RootDescriptorIndirectArgumentResolver(const RootDescriptorIndirectArgumentResolver& rhs) = delete;
		RootDescriptorIndirectArgumentResolver& operator=(const RootDescriptorIndirectArgumentResolver& rhs) = delete;

		RootDescriptorIndirectArgumentResolver(RootDescriptorIndirectArgumentResolver&& rhs) noexcept = default;
		RootDescriptorIndirectArgumentResolver& operator=(RootDescriptorIndirectArgumentResolver&& rhs) noexcept = default;

		void SetRootParameterIndex(const std::uint32_t rootParameterIndex);

		std::string CreateIndirectArgumentDescriptionInitializerString() const override;
		std::string_view GetIndirectArgumentFieldTypeString() const override;
		std::string_view GetIndirectArgumentFieldNameString() const override;
		std::optional<std::string_view> GetCustomTypeDefinitionString() const override;

	private:
		std::string mArgumentNameStr;
		std::optional<std::uint32_t> mRootParameterIndex;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	using RootCBVIndirectArgumentResolver = RootDescriptorIndirectArgumentResolver<RootDescriptorID::CBV>;
	using RootSRVIndirectArgumentResolver = RootDescriptorIndirectArgumentResolver<RootDescriptorID::SRV>;
	using RootUAVIndirectArgumentResolver = RootDescriptorIndirectArgumentResolver<RootDescriptorID::UAV>;
}
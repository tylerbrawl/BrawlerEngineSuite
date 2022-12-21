module;
#include <string>
#include <string_view>
#include <optional>

export module Brawler.IndirectArgumentResolvers:GeneralIndirectArgumentResolvers;
import :I_IndirectArgumentResolver;

namespace Brawler
{
	enum class GeneralResolverID
	{
		DRAW,
		DRAW_INDEXED,
		DISPATCH,
		INDEX_BUFFER_VIEW,
		DISPATCH_RAYS,
		DISPATCH_MESH
	};
}

namespace Brawler
{
	template <GeneralResolverID ResolverID>
	class GeneralIndirectArgumentResolver final : public I_IndirectArgumentResolver
	{
	public:
		explicit GeneralIndirectArgumentResolver(std::string argumentNameStr);

		GeneralIndirectArgumentResolver(const GeneralIndirectArgumentResolver& rhs) = delete;
		GeneralIndirectArgumentResolver& operator=(const GeneralIndirectArgumentResolver& rhs) = delete;

		GeneralIndirectArgumentResolver(GeneralIndirectArgumentResolver&& rhs) noexcept = default;
		GeneralIndirectArgumentResolver& operator=(GeneralIndirectArgumentResolver&& rhs) noexcept = default;

		std::string CreateIndirectArgumentDescriptionInitializerString() const override;
		std::string_view GetIndirectArgumentFieldTypeString() const override;
		std::string_view GetIndirectArgumentFieldNameString() const override;
		std::optional<std::string_view> GetCustomTypeDefinitionString() const override;

	private:
		std::string mArgumentNameStr;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	using DrawInstancedIndirectArgumentResolver = GeneralIndirectArgumentResolver<GeneralResolverID::DRAW>;
	using DrawIndexedInstancedIndirectArgumentResolver = GeneralIndirectArgumentResolver<GeneralResolverID::DRAW_INDEXED>;
	using DispatchIndirectArgumentResolver = GeneralIndirectArgumentResolver<GeneralResolverID::DISPATCH>;
	using IndexBufferViewIndirectArgumentResolver = GeneralIndirectArgumentResolver<GeneralResolverID::INDEX_BUFFER_VIEW>;
	using DispatchRaysIndirectArgumentResolver = GeneralIndirectArgumentResolver<GeneralResolverID::DISPATCH_RAYS>;
	using DispatchMeshIndirectArgumentResolver = GeneralIndirectArgumentResolver<GeneralResolverID::DISPATCH_MESH>;
}
module;
#include <utility>

export module Brawler.I_VirtualTexturePageState;
import Brawler.VirtualTextureLogicalPage;
import Brawler.GlobalTexturePageInfo;

export namespace Brawler
{
	template <typename DerivedType>
	class I_VirtualTexturePageState
	{
	protected:
		I_VirtualTexturePageState() = default;

	public:
		virtual ~I_VirtualTexturePageState() = default;

		I_VirtualTexturePageState(const I_VirtualTexturePageState& rhs) = delete;
		I_VirtualTexturePageState& operator=(const I_VirtualTexturePageState& rhs) = delete;

		I_VirtualTexturePageState(I_VirtualTexturePageState&& rhs) noexcept = default;
		I_VirtualTexturePageState& operator=(I_VirtualTexturePageState&& rhs) noexcept = default;

		auto OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo);
		auto OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo);

		auto OnContextFinalization();

		auto GetOperationDetails(const VirtualTextureLogicalPage& logicalPage);
	};
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename DerivedType>
	auto I_VirtualTexturePageState<DerivedType>::OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		return static_cast<DerivedType*>(this)->OnPageAddedToGlobalTexture(std::move(pageInfo));
	}

	template <typename DerivedType>
	auto I_VirtualTexturePageState<DerivedType>::OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		return static_cast<DerivedType*>(this)->OnPageRemovedFromGlobalTexture(std::move(pageInfo));
	}

	template <typename DerivedType>
	auto I_VirtualTexturePageState<DerivedType>::OnContextFinalization()
	{
		return static_cast<DerivedType*>(this)->OnContextFinalization();
	}

	template <typename DerivedType>
	auto I_VirtualTexturePageState<DerivedType>::GetOperationDetails(const VirtualTextureLogicalPage& logicalPage)
	{
		return static_cast<DerivedType*>(this)->GetOperationDetails(logicalPage);
	}
}
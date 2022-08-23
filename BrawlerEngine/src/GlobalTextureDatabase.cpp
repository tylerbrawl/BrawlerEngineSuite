module;
#include <cassert>
#include <span>
#include <unordered_map>
#include <DxDef.h>

module Brawler.GlobalTextureDatabase;
import Brawler.VirtualTexture;

namespace Brawler
{
	GlobalTextureDatabase& GlobalTextureDatabase::GetInstance()
	{
		static GlobalTextureDatabase instance{};
		return instance;
	}

	void GlobalTextureDatabase::AddVirtualTexturePage(GlobalTextureUpdateContext& context, const VirtualTextureLogicalPage& logicalPage)
	{
		assert(logicalPage.VirtualTexturePtr != nullptr);
		const DXGI_FORMAT pageDataTextureFormat = logicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetTextureFormat();

		std::expected<void, HRESULT> addPageDataResult{};
		ExecuteCallbackForFormat(pageDataTextureFormat, [&context, &logicalPage, &addPageDataResult]<DXGI_FORMAT GlobalTextureFormat>(GlobalTextureCollection<GlobalTextureFormat>& collection)
		{
			addPageDataResult = collection.AddVirtualTexturePage(context, logicalPage);
		});

		// If the page data was successfully added, or if the request was for non-combined page data,
		// then we exit. Otherwise, if we failed to add combined page data, then we need to create a
		// new GlobalTexture and try again.
		if (addPageDataResult.has_value()) [[likely]]
			return;

		const bool isCombinedPage = (logicalPage.LogicalMipLevel >= logicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage());

		if (!isCombinedPage)
			return;

		ExecuteCallbackForFormat(pageDataTextureFormat, [&context, &logicalPage]<DXGI_FORMAT GlobalTextureFormat>(GlobalTextureCollection<GlobalTextureFormat>& collection)
		{
			collection.CreateGlobalTexture();
			
			[[maybe_unused]] const std::expected<void, HRESULT> addPageDataResult = collection.AddVirtualTexturePage(context, logicalPage);
			assert(addPageDataResult.has_value());
		});
	}

	void GlobalTextureDatabase::ClearGlobalTexturePages(const std::span<const GlobalTexturePageIdentifier> pageIdentifierSpan)
	{
		// First, group each GlobalTexturePageIdentifier based on their GlobalTexture ID.
		std::unordered_map<std::uint8_t, std::vector<GlobalTexturePageIdentifier>> pageIdentifierMap{};

		for (const auto pageIdentifier : pageIdentifierSpan)
			pageIdentifierMap[pageIdentifier.GlobalTextureID].push_back(pageIdentifier);

		// TODO: Finish this!
	}
}
module;
#include <array>
#include <vector>
#include <span>
#include <memory>
#include <algorithm>
#include <optional>
#include <expected>
#include <cassert>
#include <DxDef.h>

export module Brawler.GlobalTexture;
import :GlobalTextureIndexer;
import Brawler.GlobalTexturePageSlot;
import Brawler.GlobalTextureUpdateContext;
import Brawler.GlobalTexturePageInfo;
import Brawler.GlobalTextureFormatInfo;
import Brawler.ActiveGlobalTexturePageStats;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.BindlessSRVAllocation;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.Math.MathTypes;
import Brawler.VirtualTextureLogicalPage;
import Util.General;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.GPUSceneManager;
import Brawler.GPUSceneBufferID;
import Brawler.GPUSceneBufferUpdater;
import Brawler.GlobalTexturePageIdentifier;

export namespace Brawler
{
	template <DXGI_FORMAT Format>
	class GlobalTexture
	{
	private:
		static constexpr D3D12::Texture2DBuilder TEXTURE_2D_BUILDER{ Brawler::CreateGlobalTextureBuilder<Format>() };
		static constexpr std::size_t NUM_PAGES_PER_GLOBAL_TEXTURE_ROW = (TEXTURE_2D_BUILDER.GetResourceDescription().Width / GlobalTextureFormatInfo<Format>::PADDED_PAGE_DIMENSIONS);

	public:
		static constexpr std::size_t NUM_PAGES_IN_GLOBAL_TEXTURE = (NUM_PAGES_PER_GLOBAL_TEXTURE_ROW * NUM_PAGES_PER_GLOBAL_TEXTURE_ROW);

	public:
		GlobalTexture();

		GlobalTexture(const GlobalTexture& rhs) = delete;
		GlobalTexture& operator=(const GlobalTexture& rhs) = delete;

		GlobalTexture(GlobalTexture&& rhs) noexcept = delete;
		GlobalTexture& operator=(GlobalTexture&& rhs) noexcept = delete;
		
		/// <summary>
		/// Attempts to add the logical virtual texture page specified by logicalPage to this GlobalTexture instance.
		/// The function asserts in Debug builds if the VirtualTexture's format does not match the format of this
		/// GlobalTexture.
		/// 
		/// If there are unused page slots within the GlobalTexture, then those are always preferred when adding new
		/// page data. Otherwise, the least-recently-used (LRU) non-combined page data within the GlobalTexture is replaced 
		/// with logicalPage.
		/// 
		/// We emphasize the "non-combined page data" portion of the previous sentence: This function will never replace
		/// the data for a combined page within a GlobalTexture. This is because each virtual texture must at least have
		/// its combined page in memory in order to be used in rendering. With that said, one might wonder how combined
		/// page data gets evicted from a GlobalTexture. Upon the deletion of a VirtualTexture instance, all of its previous
		/// allocations within GlobalTexture instances are revoked, including allocations made for combined pages. This is
		/// the only legal way to remove combined page data.*
		/// 
		/// The function will fail if the GlobalTexture is completely filled with combined pages. See the "returns"
		/// section for more details.
		/// 
		/// *Combined page data can be removed from a GlobalTexture via the GlobalTexture::Defragment() function, but
		/// it is expected that the returned VirtualTextureLogicalPage will subsequently be added to a different
		/// GlobalTexture.
		/// 
		/// *WARNING*: This function is *NOT* thread safe, as the thread-safe implementation would require a significant
		/// amount of atomic operations and complexity. For that reason, it is strongly recommended that GlobalTexture
		/// updates occur asynchronously in a single CPU job alongside the rest of the Brawler Engine.
		/// </summary>
		/// <param name="context">
		/// - A reference to the GlobalTextureUpdateContext instance which is recording all of the GlobalTexture updates
		///   for the current frame.
		/// </param>
		/// <param name="logicalPage">
		/// - A const VirtualTextureLogicalPage& which describes the logical virtual texture page which is to be added
		///   to the GlobalTexture.
		/// </param>
		/// <returns>
		/// If the GlobalTexture is not completely filled with combined page data, then the expected result is achieved.
		/// Otherwise, an unexpected HRESULT value is returned to indicate what went wrong. Specifically, the following
		/// HRESULT values can be returned:
		/// 
		///   - E_NOT_SUFFICIENT_BUFFER: The GlobalTexture is completely filled with combined page data.
		/// 
		/// Note that if an HRESULT is returned, then the state of the GlobalTexture instance is *NOT* modified.
		/// </returns>
		std::expected<void, HRESULT> AddVirtualTexturePage(GlobalTextureUpdateContext& context, const VirtualTextureLogicalPage& logicalPage);

		/// <summary>
		/// Attempts to remove page data from this GlobalTexture instance so that it may be moved to a different instance.
		/// This process is known as "global texture defragmentation" (hereafter "defragmentation").
		/// 
		/// During the defragmentation process, page data is temporarily removed from a GlobalTexture instance with the
		/// assumption that the extracted page data will subsequently be added to a different GlobalTexture instance of
		/// the same type/format. Since each GlobalTexture is perfectly divided into N fixed-size pages, internal fragmentation
		/// (i.e., page data within a GlobalTexture not being adjacent in memory) has no negative impact on memory
		/// consumption.
		/// 
		/// The purpose of defragmentation is to attempt to reduce the number of GlobalTexture instances which are required
		/// to be in memory. If we can move all of the page data into a few GlobalTexture instances, then any GlobalTexture
		/// instances which become empty can safely be deleted. (To prevent stutters, deletion probably shouldn't happen
		/// immediately, except under serious memory pressure).
		/// 
		/// In choosing which pages to extract first, the function will always prioritize combined pages. After all
		/// of the combined pages have been extracted, the function will prefer pages which have been used most recently.
		/// Defragmenting GlobalTexture instances in this manner makes it possible to delete a GlobalTexture instance
		/// after all of the combined pages are removed, even if some higher-LOD logical page data remains.
		/// 
		/// The function will fail if the GlobalTexture has no page data. See the "returns" section for more details.
		/// 
		/// *WARNING*: As mentioned earlier, the function assumes that extracted page data will subsequently be added to
		/// another GlobalTexture instance. The behavior is *UNDEFINED* if this is not the case!
		/// 
		/// *WARNING*: This function is *NOT* thread safe, as the thread-safe implementation would require a significant
		/// amount of atomic operations and complexity. For that reason, it is strongly recommended that GlobalTexture
		/// updates occur asynchronously in a single CPU job alongside the rest of the Brawler Engine.
		/// </summary>
		/// <param name="context">
		/// - A reference to the GlobalTextureUpdateContext instance which is recording all of the GlobalTexture updates
		///   for the current frame.
		/// </param>
		/// <returns>
		/// If the GlobalTexture has data for at least one logical virtual texture page, then the expected result is
		/// achieved, and this result is a VirtualTextureLogicalPage instance describing the page which was removed.
		/// It is assumed that this page will be subsequently added to a different GlobalTexture instance.
		/// 
		/// If there is no page data within the GlobalTexture instance, then an unexpected HRESULT value is returned
		/// to indicate what went wrong. Specifically, the following HRESULT values can be returned:
		/// 
		///   - E_PENDING: The GlobalTexture has no page data to extract.
		/// 
		/// Note that if an HRESULT is returned, then the state of the GlobalTexture instance is *NOT* modified.
		/// </returns>
		std::expected<VirtualTextureLogicalPage, HRESULT> Defragment(GlobalTextureUpdateContext& context);

		void NotifyForPageUseInCurrentFrame(const GlobalTexturePageIdentifier pageIdentifier);

		void ClearGlobalTexturePage(const GlobalTexturePageIdentifier pageIdentifier);

		ActiveGlobalTexturePageStats GetActivePageStats() const;
		std::uint64_t GetLeastRecentFrameNumber() const;

		std::uint8_t GetGlobalTextureID() const;

	private:
		Math::UInt2 GetUnflattenedCoordinates(const std::uint32_t flattenedCoordinates) const;

		GlobalTexturePageInfo CreatePageInfo(const std::size_t flattenedCoordinates) const;

	private:
		std::unique_ptr<D3D12::Texture2D> mTexture2DPtr;
		D3D12::BindlessSRVAllocation mGlobalTextureBindlessAllocation;
		GlobalTextureIndexer<0, NUM_PAGES_IN_GLOBAL_TEXTURE> mBaseIndexer;
		std::array<GlobalTexturePageSlot, NUM_PAGES_IN_GLOBAL_TEXTURE> mPageSlotArr;
		D3D12::StructuredBufferSubAllocation<GlobalTextureDescription, 1> mGlobalTextureDescriptionBufferSubAllocation;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <DXGI_FORMAT Format>
	GlobalTexture<Format>::GlobalTexture() :
		mTexture2DPtr(std::make_unique<D3D12::Texture2D>(TEXTURE_2D_BUILDER)),
		mGlobalTextureBindlessAllocation(),
		mBaseIndexer(),
		mPageSlotArr(),
		mGlobalTextureDescriptionBufferSubAllocation()
	{
		// Reserve a description within the global texture description GPU scene buffer.
		std::optional<D3D12::StructuredBufferSubAllocation<GlobalTextureDescription, 1>> descriptionBufferSubAllocation = GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::GLOBAL_TEXTURE_DESCRIPTION_BUFFER>().CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<GlobalTextureDescription, 1>>();
		assert(descriptionBufferSubAllocation.has_value() && "ERROR: We have run out of global texture description memory!");

		mGlobalTextureDescriptionBufferSubAllocation = std::move(*descriptionBufferSubAllocation);

		// Reserve a bindless SRV index for the global texture.
		mGlobalTextureBindlessAllocation = mTexture2DPtr->CreateBindlessSRV<Format>();
		
		// Create a GPUSceneBufferUpdater for this global texture description.
		static constexpr std::uint32_t GLOBAL_TEXTURE_DIMENSIONS = static_cast<std::uint32_t>(TEXTURE_2D_BUILDER.GetResourceDescription().Width);
		static constexpr std::uint32_t PADDED_PAGE_DIMENSIONS = GlobalTextureFormatInfo<Format>::PADDED_PAGE_DIMENSIONS;

		const GPUSceneBufferUpdater<GPUSceneBufferID::GLOBAL_TEXTURE_DESCRIPTION_BUFFER> gtDescriptionUpdater{ mGlobalTextureDescriptionBufferSubAllocation.GetBufferCopyRegion() };
		gtDescriptionUpdater.UpdateGPUSceneData(GlobalTextureDescription{
			.BindlessIndex = mGlobalTextureBindlessAllocation.GetBindlessSRVIndex(),
			.GlobalTextureDimensions = GLOBAL_TEXTURE_DIMENSIONS,
			.PaddedPageDimensions = PADDED_PAGE_DIMENSIONS
		});
	}

	template <DXGI_FORMAT Format>
	std::expected<void, HRESULT> GlobalTexture<Format>::AddVirtualTexturePage(GlobalTextureUpdateContext& context, const VirtualTextureLogicalPage& logicalPage)
	{
		assert(logicalPage.VirtualTexturePtr != nullptr && "ERROR: A GlobalTexture instance was provided a nullptr VirtualTexture!");
		assert(logicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetTextureFormat() == Format && "ERROR: A VirtualTexture instance was provided to a GlobalTexture instance with a different format!");

		const std::optional<std::size_t> bestIndexForInsertion{ mBaseIndexer.GetBestIndexForPageReplacement() };

		if (!bestIndexForInsertion.has_value()) [[unlikely]]
			return std::unexpected{ E_NOT_SUFFICIENT_BUFFER };

		GlobalTexturePageSlot& relevantSlot{ mPageSlotArr[*bestIndexForInsertion] };
		
		// If the GlobalTexturePageSlot at the specified index already has stored page data,
		// then we need to replace it. The GlobalTextureIndexer only returns an index to replace
		// page data if such a replacement operation would be valid (i.e., the GlobalTexturePageSlot)
		// was not already storing combined page data).
		const bool wasSlotHoldingPreviousPage = relevantSlot.HasVirtualTexturePage();

		if (wasSlotHoldingPreviousPage) [[unlikely]]
		{
			const VirtualTextureLogicalPage removedPage{ relevantSlot.RemoveVirtualTexturePage() };

			assert(removedPage.VirtualTexturePtr != nullptr);
			assert(removedPage.LogicalMipLevel < removedPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage() && "ERROR: Somehow, a combined page was prematurely removed from a GlobalTexture!");

			context.OnPageRemovedFromGlobalTexture(removedPage, CreatePageInfo(*bestIndexForInsertion));
		}

		relevantSlot.AddVirtualTexturePage(logicalPage);
		context.OnPageAddedToGlobalTexture(logicalPage, CreatePageInfo(*bestIndexForInsertion));

		if (!wasSlotHoldingPreviousPage) [[likely]]
			mBaseIndexer.UpdateForRegularPageAddition(*bestIndexForInsertion);

		const bool isInsertedPageCombinedPage = (logicalPage.LogicalMipLevel >= logicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage());

		if (isInsertedPageCombinedPage)
			mBaseIndexer.UpdateForCombinedPageAddition(*bestIndexForInsertion);

		// Update the chosen slot for being "used" in the current frame.
		mBaseIndexer.UpdateForUseInCurrentFrame(*bestIndexForInsertion);

		return std::expected<void, HRESULT>{};
	}

	template <DXGI_FORMAT Format>
	std::expected<VirtualTextureLogicalPage, HRESULT> GlobalTexture<Format>::Defragment(GlobalTextureUpdateContext& context)
	{
		const std::optional<std::size_t> bestIndexForRemoval{ mBaseIndexer.GetBestIndexForPageDefragmentation() };

		if (!bestIndexForRemoval.has_value()) [[unlikely]]
			return std::unexpected{ E_PENDING };

		GlobalTexturePageSlot& relevantSlot{ mPageSlotArr[*bestIndexForRemoval] };
		assert(relevantSlot.HasVirtualTexturePage());

		VirtualTextureLogicalPage removedPage{ relevantSlot.RemoveVirtualTexturePage() };
		context.OnPageRemovedFromGlobalTexture(removedPage, CreatePageInfo(*bestIndexForRemoval));

		mBaseIndexer.UpdateForRegularPageRemoval(*bestIndexForRemoval);

		assert(removedPage.VirtualTexturePtr != nullptr);
		const bool isRemovedPageCombinedPage = (removedPage.LogicalMipLevel >= removedPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage());

		if (isRemovedPageCombinedPage)
			mBaseIndexer.UpdateForCombinedPageRemoval(*bestIndexForRemoval);

		return std::expected<VirtualTextureLogicalPage, HRESULT>{ std::move(removedPage) };
	}

	template <DXGI_FORMAT Format>
	void GlobalTexture<Format>::NotifyForPageUseInCurrentFrame(const GlobalTexturePageIdentifier pageIdentifier)
	{
		assert(GetGlobalTextureID() == pageIdentifier.GetGlobalTextureID());

		const std::size_t flattenedPageIndex = ((static_cast<std::size_t>(pageIdentifier.GlobalTexturePageYCoordinate) * NUM_PAGES_PER_GLOBAL_TEXTURE_ROW) + static_cast<std::size_t>(pageIdentifier.GlobalTexturePageXCoordinate));
		mBaseIndexer.UpdateForUseInCurrentFrame(flattenedPageIndex);
	}

	template <DXGI_FORMAT Format>
	void GlobalTexture<Format>::ClearGlobalTexturePage(const GlobalTexturePageIdentifier pageIdentifier)
	{
		assert(pageIdentifier.GlobalTextureID == GetGlobalTextureDescriptionBufferIndex());

		const std::size_t flattenedIndex = ((static_cast<std::size_t>(pageIdentifier.GlobalTexturePageYCoordinate) * NUM_PAGES_PER_GLOBAL_TEXTURE_ROW) + static_cast<std::size_t>(pageIdentifier.GlobalTexturePageXCoordinate));

		GlobalTexturePageSlot& relevantSlot{ mPageSlotArr[flattenedIndex] };
		assert(relevantSlot.HasVirtualTexturePage());

		mBaseIndexer.UpdateForRegularPageRemoval(flattenedIndex);

		const VirtualTextureLogicalPage removedPage{ relevantSlot.RemoveVirtualTexturePage() };

		assert(removedPage.VirtualTexturePtr != nullptr);
		const bool isRemovedPageCombinedPage = (removedPage.LogicalMipLevel >= removedPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetFirstMipLevelInCombinedPage());

		if (isRemovedPageCombinedPage)
			mBaseIndexer.UpdateForCombinedPageRemoval(flattenedIndex);
	}

	template <DXGI_FORMAT Format>
	ActiveGlobalTexturePageStats GlobalTexture<Format>::GetActivePageStats() const
	{
		return mBaseIndexer.GetActivePageStats();
	}

	template <DXGI_FORMAT Format>
	std::uint64_t GlobalTexture<Format>::GetLeastRecentFrameNumber() const
	{
		return mBaseIndexer.GetLeastRecentFrameNumber();
	}

	template <DXGI_FORMAT Format>
	std::uint8_t GlobalTexture<Format>::GetGlobalTextureID() const
	{
		// The ID for a GlobalTexture is the same as its index within the GlobalTexture description
		// GPUSceneBuffer.

		return static_cast<std::uint8_t>(mGlobalTextureDescriptionBufferSubAllocation.GetOffsetFromBufferStart() / sizeof(GlobalTextureDescription));
	}

	template <DXGI_FORMAT Format>
	Math::UInt2 GlobalTexture<Format>::GetUnflattenedCoordinates(const std::uint32_t flattenedCoordinates) const
	{
		return Math::UInt2{
			(flattenedCoordinates % NUM_PAGES_PER_GLOBAL_TEXTURE_ROW),
			(flattenedCoordinates / NUM_PAGES_PER_GLOBAL_TEXTURE_ROW)
		};
	}

	template <DXGI_FORMAT Format>
	GlobalTexturePageInfo GlobalTexture<Format>::CreatePageInfo(const std::size_t flattenedCoordinates) const
	{
		static constexpr std::uint32_t PADDED_PAGE_DIMENSIONS = GlobalTextureFormatInfo<Format>::PADDED_PAGE_DIMENSIONS;
		
		assert(flattenedCoordinates <= std::numeric_limits<std::uint32_t>::max());
		const Math::UInt2 unflattenedPageCoordinates{ GetUnflattenedCoordinates(static_cast<std::uint32_t>(flattenedCoordinates)) };

		assert(mTexture2DPtr != nullptr);

		const GlobalTexturePageInfo::InitInfo<Format> initInfo{
			.GlobalTexture2D{ *mTexture2DPtr },
			.PageCoordinates{ unflattenedPageCoordinates },
			.GlobalTextureID = GetGlobalTextureID()
		};

		return GlobalTexturePageInfo{ initInfo };
	}
}
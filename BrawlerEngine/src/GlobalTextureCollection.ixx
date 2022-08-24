module;
#include <vector>
#include <memory>
#include <span>
#include <expected>
#include <cassert>
#include <ranges>
#include <algorithm>
#include <DxDef.h>

export module Brawler.GlobalTextureCollection;
import Brawler.GlobalTexture;
import Brawler.GlobalTextureUpdateContext;
import Brawler.ActiveGlobalTexturePageStats;
import Brawler.VirtualTextureLogicalPage;
import Util.Engine;
import Util.General;
import Brawler.GlobalTexturePageIdentifier;

export namespace Brawler
{
	template <DXGI_FORMAT Format>
	class GlobalTextureCollection
	{
	private:
		struct PendingGlobalTextureDeletion
		{
			std::unique_ptr<GlobalTexture<Format>> GlobalTexturePtr;
			std::uint64_t SafeDeletionFrameNumber;
		};

	public:
		GlobalTextureCollection();

		GlobalTextureCollection(const GlobalTextureCollection& rhs) = delete;
		GlobalTextureCollection& operator=(const GlobalTextureCollection& rhs) = delete;

		GlobalTextureCollection(GlobalTextureCollection&& rhs) noexcept = default;
		GlobalTextureCollection& operator=(GlobalTextureCollection&& rhs) noexcept = default;

		void CreateGlobalTexture();
		std::size_t GetCurrentGlobalTextureCount() const;

		std::expected<void, HRESULT> AddVirtualTexturePage(GlobalTextureUpdateContext& context, const VirtualTextureLogicalPage& logicalPage);

		void TryDefragmentGlobalTexturesWeak(GlobalTextureUpdateContext& context);
		void TryDefragmentGlobalTexturesStrong(GlobalTextureUpdateContext& context);

		void ClearGlobalTexturePages(const std::span<const GlobalTexturePageIdentifier> pageIdentifierSpan);

		bool HasGlobalTextureID(const std::uint8_t desiredID) const;

	private:
		template <bool AllowHighLODDeletion>
		void TryDefragmentGlobalTextures(GlobalTextureUpdateContext& context);

		template <bool AllowHighLODDeletion>
		bool TryDefragmentGlobalTexture(GlobalTextureUpdateContext& context, GlobalTexture<Format>& globalTexture);

	private:
		std::vector<std::unique_ptr<GlobalTexture<Format>>> mGlobalTexturePtrArr;
		std::vector<PendingGlobalTextureDeletion> mPendingDeletionArr;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <DXGI_FORMAT Format>
	template <bool AllowHighLODDeletion>
	void GlobalTextureCollection<Format>::TryDefragmentGlobalTextures(GlobalTextureUpdateContext& context)
	{
		std::erase_if(mPendingDeletionArr, [] (const PendingGlobalTextureDeletion& pendingDeletion)
		{
			return (pendingDeletion.SafeDeletionFrameNumber <= Util::Engine::GetTrueFrameNumber());
		});

		std::erase_if(mGlobalTexturePtrArr, [this, &context] (std::unique_ptr<GlobalTexture<Format>>& globalTexturePtr)
		{
			if (TryDefragmentGlobalTexture<AllowHighLODDeletion>(context, *globalTexturePtr))
			{
				mPendingDeletionArr.push_back(PendingGlobalTextureDeletion{
					.GlobalTexturePtr{std::move(globalTexturePtr)},
					.SafeDeletionFrameNumber = (Util::Engine::GetTrueFrameNumber() + (Util::Engine::MAX_FRAMES_IN_FLIGHT + 1))
					});

				return true;
			}

			return false;
		});
	}

	template <DXGI_FORMAT Format>
	template <bool AllowHighLODDeletion>
	bool GlobalTextureCollection<Format>::TryDefragmentGlobalTexture(GlobalTextureUpdateContext& context, GlobalTexture<Format>& globalTexture)
	{
		// The number of pages we need to remove from globalTexture varies based
		// on whether or not we allow high-LOD virtual texture page data (that is, page
		// data which is not for a combined page) to be lost.
		std::uint32_t numFreeSlotsRequiredForDeletion = 0;

		if constexpr (AllowHighLODDeletion)
			numFreeSlotsRequiredForDeletion = globalTexture.GetActivePageStats().NumPagesFilledForCombinedPages;
		else
			numFreeSlotsRequiredForDeletion = globalTexture.GetActivePageStats().NumPagesFilled;

		std::vector<GlobalTexture<Format>*> candidateTexturePtrArr{};
		std::uint32_t numPagesAvailableInCandidateTextures = 0;

		for (const auto& globalTexturePtr : mGlobalTexturePtrArr)
		{
			if (globalTexturePtr.get() != &globalTexture)
			{
				candidateTexturePtrArr.push_back(globalTexturePtr.get());

				// Similarly, the number of slots within candidate GlobalTexture instances which
				// could potentially store the data we are moving out of globalTexture varies based
				// on AllowHighLODDeletion.
				if constexpr (AllowHighLODDeletion)
					numPagesAvailableInCandidateTextures += (GlobalTexture<Format>::NUM_PAGES_IN_GLOBAL_TEXTURE - globalTexturePtr->GetActivePageStats().NumPagesFilledForCombinedPages);
				else
					numPagesAvailableInCandidateTextures += (GlobalTexture<Format>::NUM_PAGES_IN_GLOBAL_TEXTURE - globalTexturePtr->GetActivePageStats().NumPagesFilled);
			}
		}

		if (numPagesAvailableInCandidateTextures < numFreeSlotsRequiredForDeletion)
			return false;

		// If we have enough slots free in the candidate GlobalTexture instances, then
		// we move all of the relevant texture data over. Even if we allow the loss of
		// high-LOD virtual texture pages, we still prefer not to lose them. So, we first
		// go through all of the candidate GlobalTextures and fill in empty slots before
		// we begin replacing page data.

		const auto transferPageDataLambda = [&context, &globalTexture] (GlobalTexture<Format>& candidateTexture)
		{
			const std::expected<VirtualTextureLogicalPage, HRESULT> extractedLogicalPage{ globalTexture.Defragment(context) };

			if (extractedLogicalPage.has_value()) [[likely]]
			{
				[[maybe_unused]] const std::expected<void, HRESULT> addPageResult{ candidateTexture.AddVirtualTexturePage(context, *extractedLogicalPage) };
				assert(addPageResult.has_value());

				return true;
			}

				// The only recognized HRESULT error is E_PENDING, which implies that
				// globalTexture no longer has any page data left to transfer.
			assert(extractedLogicalPage.error() == E_PENDING && "ERROR: GlobalTexture::Defragment() returned an unexpected error value!");
			return false;
		};

		for (const auto candidateTexturePtr : candidateTexturePtrArr)
		{
			while (candidateTexturePtr->GetActivePageStats().NumPagesFilled < GlobalTexture<Format>::NUM_PAGES_IN_GLOBAL_TEXTURE)
			{
				const bool wasPageDataTransferred = transferPageDataLambda(*candidateTexturePtr);

				if (!wasPageDataTransferred)
					return true;
			}
		}

		if constexpr (!AllowHighLODDeletion)
		{
			// We shouldn't get here unless we allow the deletion of high-LOD virtual texture
			// pages.
			assert(false);
			std::unreachable();

			return false;
		}

		for (const auto candidateTexturePtr : candidateTexturePtrArr)
		{
			while (candidateTexturePtr->GetActivePageStats().NumPagesFilledForCombinedPages < GlobalTexture<Format>::NUM_PAGES_IN_GLOBAL_TEXTURE)
			{
				const bool wasPageDataTransferred = transferPageDataLambda(*candidateTexturePtr);

				if (!wasPageDataTransferred)
					return true;
			}
		}

		assert(false);
		std::unreachable();

		return false;
	}

	template <DXGI_FORMAT Format>
	GlobalTextureCollection<Format>::GlobalTextureCollection() :
		mGlobalTexturePtrArr(),
		mPendingDeletionArr()
	{
		// Always create a GlobalTexture instance immediately. That way, we don't need to worry
		// about random stutters when loading in the first combined page data.
		CreateGlobalTexture();
	}

	template <DXGI_FORMAT Format>
	void GlobalTextureCollection<Format>::CreateGlobalTexture()
	{
		mGlobalTexturePtrArr.push_back(std::make_unique<GlobalTexture<Format>>());
	}

	template <DXGI_FORMAT Format>
	std::size_t GlobalTextureCollection<Format>::GetCurrentGlobalTextureCount() const
	{
		return mGlobalTexturePtrArr.size();
	}

	template <DXGI_FORMAT Format>
	std::expected<void, HRESULT> GlobalTextureCollection<Format>::AddVirtualTexturePage(GlobalTextureUpdateContext& context, const VirtualTextureLogicalPage& logicalPage)
	{
		// First, try to add the VirtualTextureLogicalPage to a GlobalTexture which has an empty
		// slot.
		for (const auto& globalTexturePtr : mGlobalTexturePtrArr)
		{
			if (globalTexturePtr->GetActivePageStats().NumPagesFilled < GlobalTexture<Format>::NUM_PAGES_IN_GLOBAL_TEXTURE) [[likely]]
			{
				[[maybe_unused]] const std::expected<void, HRESULT> addPageResult{ globalTexturePtr->AddVirtualTexturePage(context, logicalPage) };

				// We don't expect the function to fail if it has an open slot.
				assert(addPageResult.has_value());

				return std::expected<void, HRESULT>{};
			}
		}

		// If that failed, then try to replace the data in a GlobalTexture slot which is not being used
		// for combined page data. Ideally, we would replace page data only when we know that we won't
		// need its contents to be present on the GPU anymore. If we knew that, however, then the
		// first loop above would have already assigned logicalPage to a GlobalTexture slot.
		//
		// The next best thing, then, would be to use a least-recently-used (LRU) replacement policy.
		// So, we will sort the GlobalTexture instances based on their least recent frame number.
		std::vector<GlobalTexture<Format>*> sortedGlobalTexturePtrArr{};

		for (const auto& globalTexturePtr : mGlobalTexturePtrArr)
			sortedGlobalTexturePtrArr.push_back(globalTexturePtr.get());

		std::ranges::sort(sortedGlobalTexturePtrArr, [] (const GlobalTexture<Format>* const lhs, const GlobalTexture<Format>* const rhs)
		{
			return (lhs->GetLeastRecentFrameNumber() < rhs->GetLeastRecentFrameNumber());
		});

		for (const auto sortedGlobalTexturePtr : sortedGlobalTexturePtrArr)
		{
			const std::expected<void, HRESULT> addPageResult{ sortedGlobalTexturePtr->AddVirtualTexturePage(context, logicalPage) };

			if (addPageResult.has_value()) [[likely]]
				return std::expected<void, HRESULT>{};

			// Only E_NOT_SUFFICIENT_BUFFER is expected as an error code. In that case, we just try
			// the next GlobalTexture instance.
			assert(addPageResult.error() == E_NOT_SUFFICIENT_BUFFER && "ERROR: GlobalTexture::AddVirtualTexturePage() returned an unexpected error value!");
		}

		return std::unexpected{ E_NOT_SUFFICIENT_BUFFER };
	}

	template <DXGI_FORMAT Format>
	void GlobalTextureCollection<Format>::TryDefragmentGlobalTexturesWeak(GlobalTextureUpdateContext& context)
	{
		TryDefragmentGlobalTextures<false>(context);
	}

	template <DXGI_FORMAT Format>
	void GlobalTextureCollection<Format>::TryDefragmentGlobalTexturesStrong(GlobalTextureUpdateContext& context)
	{
		TryDefragmentGlobalTextures<true>(context);
	}

	template <DXGI_FORMAT Format>
	void GlobalTextureCollection<Format>::ClearGlobalTexturePages(const std::span<const GlobalTexturePageIdentifier> pageIdentifierSpan)
	{
		// We assume that each std::span passed here contains only identifiers for pages
		// belonging to a single GlobalTexture. This is fine because the function is only called
		// internally by GlobalTextureDatabase.
		if (pageIdentifierSpan.empty()) [[unlikely]]
			return;

		const std::uint8_t relevantGlobalTextureID = pageIdentifierSpan[0].GlobalTextureID;

		if constexpr (Util::General::IsDebugModeEnabled())
		{
			assert(HasGlobalTextureID(relevantGlobalTextureID) && "ERROR: GlobalTextureCollection::ClearGlobalTexturePages() was called for a GlobalTexture ID which a collection instance did not own!");

			for (const auto pageIdentifier : pageIdentifierSpan | std::views::drop(1))
				assert(pageIdentifer.GlobalTextureID == relevantGlobalTextureID && "ERROR: GlobalTextureCollection::ClearGlobalTexturePages() assumes that all of the provided GlobalTexturePageIdentifier instances passed to it are for the same GlobalTexture! (This is done for performance.)");
		}

		for (const auto& globalTexturePtr : mGlobalTexturePtrArr)
		{
			if (globalTexturePtr->GetGlobalTextureID() == relevantGlobalTextureID)
			{
				for (const auto pageIdentifier : pageIdentifierSpan)
					globalTexturePtr->ClearGlobalTexturePage(pageIdentifier);

				return;
			}
		}
	}

	template <DXGI_FORMAT Format>
	bool GlobalTextureCollection<Format>::HasGlobalTextureID(const std::uint8_t desiredID) const
	{
		// We really don't expect there to be a lot of GlobalTexture instances being created, so
		// doing a linear search like this is probably going to be a lot faster than using a
		// map.
		for (const auto& globalTexturePtr : mGlobalTexturePtrArr)
		{
			if (globalTexturePtr->GetGlobalTextureID() == desiredID)
				return true;
		}

		return false;
	}
}
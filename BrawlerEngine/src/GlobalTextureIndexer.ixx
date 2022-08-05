module;
#include <cstddef>
#include <optional>
#include <cassert>

export module Brawler.GlobalTexture:GlobalTextureIndexer;
import Brawler.ActiveGlobalTexturePageStats;
import Util.Engine;

export namespace Brawler
{
	template <std::size_t StartingIndex, std::size_t NumPagesInRange>
	class GlobalTextureIndexer
	{
	private:
		enum class SegmentChoice
		{
			LHS,
			RHS
		};

	private:
		template <std::size_t OtherIndex, std::size_t OtherNumPages>
		friend class GlobalTextureIndexer;

	private:
		static constexpr std::size_t LHS_STARTING_INDEX = StartingIndex;
		static constexpr std::size_t LHS_SIZE = (NumPagesInRange / 2);

		static constexpr std::size_t RHS_STARTING_INDEX = (LHS_STARTING_INDEX + LHS_SIZE);
		static constexpr std::size_t RHS_SIZE = (NumPagesInRange - LHS_SIZE);

	public:
		GlobalTextureIndexer() = default;

		GlobalTextureIndexer(const GlobalTextureIndexer& rhs) = delete;
		GlobalTextureIndexer& operator=(const GlobalTextureIndexer& rhs) = delete;

		GlobalTextureIndexer(GlobalTextureIndexer&& rhs) noexcept = delete;
		GlobalTextureIndexer& operator=(GlobalTextureIndexer&& rhs) noexcept = delete;

		std::optional<std::size_t> GetBestIndexForPageReplacement() const;
		std::optional<std::size_t> GetBestIndexForPageDefragmentation() const;

		void UpdateForRegularPageAddition(const std::size_t pageIndex);
		void UpdateForCombinedPageAddition(const std::size_t pageIndex);

		void UpdateForRegularPageRemoval(const std::size_t pageIndex);
		void UpdateForCombinedPageRemoval(const std::size_t pageIndex);

		void UpdateForUseInCurrentFrame(const std::size_t pageIndex);

	private:
		void UpdateForUseInCurrentFrameIMPL(const std::size_t pageIndex, const std::uint64_t currFrameNumber);

		static SegmentChoice ChooseSegment(const std::size_t pageIndex);

	private:
		// To prevent the CPU cache from getting completely obliterated, we store
		// the ActiveGlobalTexturePageStats for each level within the same GlobalTextureIndexer
		// instances; that way, their values can be compared in adjacent memory.

		ActiveGlobalTexturePageStats mLHSPageStats;
		ActiveGlobalTexturePageStats mRHSPageStats;
		[[no_unique_address]] GlobalTextureIndexer<LHS_STARTING_INDEX, LHS_SIZE> mLHSIndexer;
		[[no_unique_address]] GlobalTextureIndexer<RHS_STARTING_INDEX, RHS_SIZE> mRHSIndexer;
	};

	template <std::size_t StartingIndex>
	class GlobalTextureIndexer<StartingIndex, 1>
	{
	public:
		GlobalTextureIndexer() = default;

		GlobalTextureIndexer(const GlobalTextureIndexer& rhs) = delete;
		GlobalTextureIndexer& operator=(const GlobalTextureIndexer& rhs) = delete;

		GlobalTextureIndexer(GlobalTextureIndexer&& rhs) noexcept = delete;
		GlobalTextureIndexer& operator=(GlobalTextureIndexer&& rhs) noexcept = delete;

		std::optional<std::size_t> GetBestIndexForPageReplacement() const
		{
			static constexpr std::optional<std::size_t> RETURNED_INDEX{ StartingIndex };
			return RETURNED_INDEX;
		}

		std::optional<std::size_t> GetBestIndexForPageDefragmentation() const
		{
			static constexpr std::optional<std::size_t> RETURNED_INDEX{ StartingIndex };
			return RETURNED_INDEX;
		}

		void UpdateForRegiularPageAddition(const std::size_t pageIndex) const
		{}

		void UpdateForCombinedPageAddition(const std::size_t pageIndex) const
		{}

		void UpdateForRegularPageRemoval(const std::size_t pageIndex) const
		{}

		void UpdateForCombinedPageRemoval(const std::size_t pageIndex) const
		{}

	private:
		void UpdateForUseInCurrentFrameIMPL(const std::size_t pageIndex, const std::uint64_t currFrameNumber) const
		{}
	};
}

// -----------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <std::size_t StartingIndex, std::size_t NumPagesInRange>
	std::optional<std::size_t> GlobalTextureIndexer<StartingIndex, NumPagesInRange>::GetBestIndexForPageReplacement() const
	{
		// GlobalTexture instances can have a lot (> 10,000) of elements, so we want to
		// implement our search to be as fast as possible. We traditionally choose to
		// replace pages based on the following heuristics:
		//
		//   1. Replace empty/unused pages in a GlobalTexture first.
		//   2. Replace the least-recently-used (LRU) GlobalTexture page with the new
		//      page.
		//
		// So, we design our search to get a page which matches these results in a
		// short amount of time.

		if (mLHSPageStats.NumPagesFilled != LHS_SIZE)
			return mLHSIndexer.GetBestIndexForPageReplacement();

		if (mRHSPageStats.NumPagesFilled != RHS_SIZE)
			return mRHSIndexer.GetBestIndexForPageReplacement();

		// If both sides report that they have reached their capacity, then make sure that
		// we have pages which can be replaced, and prefer to replace pages on the side which
		// was used least recently.
		const bool wasLHSUsedMoreRecently = (mLHSPageStats.MostRecentUsedFrameNumber > mRHSPageStats.MostRecentUsedFrameNumber);

		if (wasLHSUsedMoreRecently && (mRHSPageStats.NumPagesFilledForCombinedPages != RHS_SIZE))
			return mRHSIndexer.GetBestIndexForPageReplacement();

		if (mLHSPageStats.NumPagesFilledForCombinedPages != LHS_SIZE)
			return mLHSIndexer.GetBestIndexForPageReplacement();

		// If both sides are completely filled with only combined pages, then we cannot replace 
		// any page, since those are needed.
		return std::optional<std::size_t>{};
	}

	template <std::size_t StartingIndex, std::size_t NumPagesInRange>
	std::optional<std::size_t> GlobalTextureIndexer<StartingIndex, NumPagesInRange>::GetBestIndexForPageDefragmentation() const
	{
		// When defragmenting GlobalTexture instances, we want to move pages out of a
		// GlobalTexture based on the following heuristics:
		//
		//   1. Remove all combined pages first, since these need to be present in at
		//      least one GlobalTexture.
		//   2. Remove the most-recently-used (MRU) GlobalTexture page.
		//
		// Note that when we say "remove" here, we do so with the assumption that the
		// actual page data is going to be copied into a different GlobalTexture instance
		// on the GPU. This would correspond to the creation of a 
		// GlobalTexturePageTransferRequest.

		if (mLHSPageStats.NumPagesFilled == 0 && mRHSPageStats.NumPagesFilled == 0)
			return std::optional<std::size_t>{};

		if (mLHSPageStats.NumPagesFilledForCombinedPages > 0)
			return mLHSIndexer.GetBestIndexForPageDefragmentation();

		if (mRHSPageStats.NumPagesFilledForCombinedPages > 0)
			return mRHSIndexer.GetBestIndexForPageDefragmentation();

		// Only once we have ensured that the combined pages have been transferred can we
		// begin to move higher-LOD page data out of GlobalTexture instances. We prefer
		// moving pages which were used most recently; that way, if we cannot move a
		// page into a different GlobalTexture and we need to delete its containing
		// GlobalTexture instance to free VRAM, we can assume that we made the right choice.
		return (mLHSPageStats.MostRecentUsedFrameNumber > mRHSPageStats.MostRecentUsedFrameNumber ? mLHSIndexer.GetBestIndexForPageDefragmentation() : mRHSIndexer.GetBestIndexForPageDefragmentation());
	}

	template <std::size_t StartingIndex, std::size_t NumPagesInRange>
	void GlobalTextureIndexer<StartingIndex, NumPagesInRange>::UpdateForRegularPageAddition(const std::size_t pageIndex)
	{
		const SegmentChoice choice = ChooseSegment(pageIndex);

		if (choice == SegmentChoice::LHS)
		{
			++(mLHSPageStats.NumPagesFilled);
			mLHSIndexer.UpdateForRegularPageAddition(pageIndex);
		}
		else
		{
			++(mRHSPageStats.NumPagesFilled);
			mRHSIndexer.UpdateForRegularPageAddition(pageIndex);
		}
	}

	template <std::size_t StartingIndex, std::size_t NumPagesInRange>
	void GlobalTextureIndexer<StartingIndex, NumPagesInRange>::UpdateForCombinedPageAddition(const std::size_t pageIndex)
	{
		const SegmentChoice choice = ChooseSegment(pageIndex);

		if (choice == SegmentChoice::LHS)
		{
			++(mLHSPageStats.NumPagesFilled);
			++(mLHSPageStats.NumPagesFilledForCombinedPages);

			mLHSIndexer.UpdateForCombinedPageAddition(pageIndex);
		}
		else
		{
			++(mRHSPageStats.NumPagesFilled);
			++(mRHSPageStats.NumPagesFilledForCombinedPages);

			mRHSIndexer.UpdateForCombinedPageAddition(pageIndex);
		}
	}

	template <std::size_t StartingIndex, std::size_t NumPagesInRange>
	void GlobalTextureIndexer<StartingIndex, NumPagesInRange>::UpdateForRegularPageRemoval(const std::size_t pageIndex)
	{
		const SegmentChoice choice = ChooseSegment(pageIndex);

		if (choice == SegmentChoice::LHS)
		{
			--(mLHSPageStats.NumPagesFilled);
			mLHSIndexer.UpdateForRegularPageRemoval(pageIndex);
		}
		else
		{
			--(mRHSPageStats.NumPagesFilled);
			mRHSIndexer.UpdateForRegularPageRemoval(pageIndex);
		}
	}

	template <std::size_t StartingIndex, std::size_t NumPagesInRange>
	void GlobalTextureIndexer<StartingIndex, NumPagesInRange>::UpdateForCombinedPageRemoval(const std::size_t pageIndex)
	{
		const SegmentChoice choice = ChooseSegment(pageIndex);

		if (choice == SegmentChoice::LHS)
		{
			--(mLHSPageStats.NumPagesFilled);
			--(mLHSPageStats.NumPagesFilledForCombinedPages);

			mLHSIndexer.UpdateForCombinedPageRemoval(pageIndex);
		}
		else
		{
			--(mRHSPageStats.NumPagesFilled);
			--(mRHSPageStats.NumPagesFilledForCombinedPages);

			mRHSIndexer.UpdateForCombinedPageRemoval(pageIndex);
		}
	}

	template <std::size_t StartingIndex, std::size_t NumPagesInRange>
	void GlobalTextureIndexer<StartingIndex, NumPagesInRange>::UpdateForUseInCurrentFrame(const std::size_t pageIndex)
	{
		UpdateForUseInCurrentFrameIMPL(pageIndex, Util::Engine::GetCurrentFrameNumber());
	}

	template <std::size_t StartingIndex, std::size_t NumPagesInRange>
	void GlobalTextureIndexer<StartingIndex, NumPagesInRange>::UpdateForUseInCurrentFrameIMPL(const std::size_t pageIndex, const std::uint64_t currFrameNumber)
	{
		const SegmentChoice choice = ChooseSegment(pageIndex);

		if (choice == SegmentChoice::LHS && currFrameNumber > mLHSPageStats.MostRecentUsedFrameNumber)
		{
			mLHSPageStats.MostRecentUsedFrameNumber = currFrameNumber;
			mLHSIndexer.UpdateForUseInCurrentFrameIMPL(pageIndex, currFrameNumber);
		}
		else if (choice == SegmentChoice::RHS && currFrameNumber > mRHSPageStats.MostRecentUsedFrameNumber)
		{
			mRHSPageStats.MostRecentUsedFrameNumber = currFrameNumber;
			mRHSIndexer.UpdateForUseInCurrentFrameIMPL(pageIndex, currFrameNumber);
		}
	}

	template <std::size_t StartingIndex, std::size_t NumPagesInRange>
	GlobalTextureIndexer<StartingIndex, NumPagesInRange>::SegmentChoice GlobalTextureIndexer<StartingIndex, NumPagesInRange>::ChooseSegment(const std::size_t pageIndex)
	{
		assert(pageIndex < (StartingIndex + NumPagesInRange));

		static constexpr std::size_t MAX_LHS_INDEX = (LHS_STARTING_INDEX + LHS_SIZE - 1);
		return (LHS_STARTING_INDEX <= pageIndex && pageIndex <= MAX_LHS_INDEX ? SegmentChoice::LHS : SegmentChoice::RHS);
	}
}
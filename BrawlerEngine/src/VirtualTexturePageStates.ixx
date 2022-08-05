module;
#include <cassert>
#include <optional>
#include <memory>

export module Brawler.VirtualTexturePageStates;
import Brawler.I_VirtualTexturePageState;
import Brawler.VirtualTextureLogicalPage;
import Brawler.GlobalTexturePageInfo;
import Brawler.GlobalTexturePageUploadRequest;
import Brawler.GlobalTexturePageRemovalRequest;
import Brawler.GlobalTexturePageTransferRequest;

export namespace Brawler
{
	class VirtualTexturePageNeutralState final : public I_VirtualTexturePageState<VirtualTexturePageNeutralState>
	{
	public:
		VirtualTexturePageNeutralState() = default;

		auto OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo);
		auto OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo);

		auto OnContextFinalization();

		auto GetOperationDetails(const VirtualTextureLogicalPage& logicalPage);
	};
	
	class VirtualTexturePageAdditionState final : public I_VirtualTexturePageState<VirtualTexturePageAdditionState>
	{
	public:
		explicit VirtualTexturePageAdditionState(GlobalTexturePageInfo&& destPageInfo);

		auto OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo);
		auto OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo);

		auto OnContextFinalization();

		auto GetOperationDetails(const VirtualTextureLogicalPage& logicalPage);

	private:
		GlobalTexturePageInfo mDestPageInfo;
	};

	class VirtualTexturePagePendingRemovalState final : public I_VirtualTexturePageState<VirtualTexturePagePendingRemovalState>
	{
	public:
		explicit VirtualTexturePagePendingRemovalState(GlobalTexturePageInfo&& srcPageInfo);

		auto OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo);
		auto OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo);

		auto OnContextFinalization();

		auto GetOperationDetails(const VirtualTextureLogicalPage& logicalPage);

	private:
		GlobalTexturePageInfo mSrcPageInfo;
	};

	class VirtualTexturePageCommittedRemovalState final : public I_VirtualTexturePageState<VirtualTexturePageCommittedRemovalState>
	{
	public:
		VirtualTexturePageCommittedRemovalState() = default;

		auto OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo);
		auto OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo);

		auto OnContextFinalization();

		auto GetOperationDetails(const VirtualTextureLogicalPage& logicalPage);
	};

	class VirtualTexturePageTransferState final : public I_VirtualTexturePageState<VirtualTexturePageTransferState>
	{
	public:
		VirtualTexturePageTransferState(GlobalTexturePageInfo&& destPageInfo, GlobalTexturePageInfo&& srcPageInfo);

		auto OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo);
		auto OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo);

		auto OnContextFinalization();

		auto GetOperationDetails(const VirtualTextureLogicalPage& logicalPage);

	private:
		GlobalTexturePageInfo mDestPageInfo;
		GlobalTexturePageInfo mSrcPageInfo;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	auto VirtualTexturePageNeutralState::OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		return std::optional<VirtualTexturePageAdditionState>{ std::in_place, std::move(pageInfo) };
	}

	auto VirtualTexturePageNeutralState::OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		return std::optional<VirtualTexturePagePendingRemovalState>{ std::in_place, std::move(pageInfo) };
	}

	auto VirtualTexturePageNeutralState::OnContextFinalization()
	{
		return std::optional<VirtualTexturePageNeutralState>{};
	}

	auto VirtualTexturePageNeutralState::GetOperationDetails(const VirtualTextureLogicalPage& logicalPage)
	{
		assert(false && "ERROR: An attempt was made to proceed with GlobalTexture operations for a virtual texture page with no pending changes!");
		std::unreachable();

		return std::optional<VirtualTexturePageNeutralState>{};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	VirtualTexturePageAdditionState::VirtualTexturePageAdditionState(GlobalTexturePageInfo&& destPageInfo) :
		I_VirtualTexturePageState<VirtualTexturePageAdditionState>(),
		mDestPageInfo(std::move(destPageInfo))
	{}

	auto VirtualTexturePageAdditionState::OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		assert(false && "ERROR: An attempt was made to add a virtual texture page to more than one GlobalTexture texel!");
		std::unreachable();

		return std::optional<VirtualTexturePageAdditionState>{};
	}

	auto VirtualTexturePageAdditionState::OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		// If a virtual texture page was going to be added, but it was later decided that the
		// page should be removed, then we revert back to the neutral state, as if no request
		// was ever made for the page.
		return std::optional<VirtualTexturePageNeutralState>{ std::in_place };
	}

	auto VirtualTexturePageAdditionState::OnContextFinalization()
	{
		return std::optional<VirtualTexturePageAdditionState>{};
	}

	auto VirtualTexturePageAdditionState::GetOperationDetails(const VirtualTextureLogicalPage& logicalPage)
	{
		return std::make_unique<GlobalTexturePageUploadRequest>(logicalPage, std::move(mDestPageInfo));
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	VirtualTexturePagePendingRemovalState::VirtualTexturePagePendingRemovalState(GlobalTexturePageInfo&& srcPageInfo) :
		I_VirtualTexturePageState<VirtualTexturePagePendingRemovalState>(),
		mSrcPageInfo(std::move(srcPageInfo))
	{}

	auto VirtualTexturePagePendingRemovalState::OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		// If a virtual texture page was pending removal, but was later assigned to be added to
		// a different GlobalTexture page, then we instead enter the transfer state; that is, we
		// will be transferring the page data from the GlobalTexture page referenced by mSrcPageInfo
		// to the page referenced by pageInfo.
		//
		// Of course, this assumes that pageInfo is not referring to the same GlobalTexture page
		// as mSrcPageInfo. If that is the case, then we can just revert back to the neutral state,
		// since it results in effectively no changes being applied to the GlobalTexture. Since we
		// cannot change the return type of this function, this check is done in 
		// VirtualTexturePageTransferState::OnContextFinalization().
		return std::optional<VirtualTexturePageTransferState>{ std::in_place, std::move(pageInfo), std::move(mSrcPageInfo) };
	}

	auto VirtualTexturePagePendingRemovalState::OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		assert(false && "ERROR: An attempt was made to remove a virtual texture page from more than one GlobalTexture texel!");
		std::unreachable();

		return std::optional<VirtualTexturePagePendingRemovalState>{};
	}

	auto VirtualTexturePagePendingRemovalState::OnContextFinalization()
	{
		// If a virtual texture page is pending removal and is never assigned to another GlobalTexture
		// page, then it really is going to be removed. In that case, we move to the committed removal
		// state.
		return std::optional<VirtualTexturePageCommittedRemovalState>{ std::in_place };
	}

	auto VirtualTexturePagePendingRemovalState::GetOperationDetails(const VirtualTextureLogicalPage& logicalPage)
	{
		// We shouldn't ever get here.
		assert(false && "ERROR: Somehow, a virtual texture page was left in the pending removal state after GlobalTextureUpdateContext finalization!");
		std::unreachable();

		return std::optional<VirtualTexturePagePendingRemovalState>{};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	auto VirtualTexturePageCommittedRemovalState::OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		assert(false && "ERROR: Once a virtual texture page has entered the committed removal state, no additional operations should be applied for it!");
		std::unreachable();

		return std::optional<VirtualTexturePageCommittedRemovalState>{};
	}

	auto VirtualTexturePageCommittedRemovalState::OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		assert(false && "ERROR: Once a virtual texture page has entered the committed removal state, no additional operations should be applied for it!");
		std::unreachable();

		return std::optional<VirtualTexturePageCommittedRemovalState>{};
	}

	auto VirtualTexturePageCommittedRemovalState::OnContextFinalization()
	{
		// This isn't really necessary, but we shouldn't be calling this function anyways if we are
		// in this state. By using std::unreachable(), we can potentially optimize the generated code.
		assert(false && "ERROR: Once a virtual texture page has entered the committed removal state, no additional operations should be applied for it!");
		std::unreachable();
		
		return std::optional<VirtualTexturePageCommittedRemovalState>{};
	}

	auto VirtualTexturePageCommittedRemovalState::GetOperationDetails(const VirtualTextureLogicalPage& logicalPage)
	{
		return std::make_unique<GlobalTexturePageRemovalRequest>(logicalPage);
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	VirtualTexturePageTransferState::VirtualTexturePageTransferState(GlobalTexturePageInfo&& destPageInfo, GlobalTexturePageInfo&& srcPageInfo) :
		I_VirtualTexturePageState<VirtualTexturePageTransferState>(),
		mDestPageInfo(std::move(destPageInfo)),
		mSrcPageInfo(std::move(srcPageInfo))
	{}

	auto VirtualTexturePageTransferState::OnPageAddedToGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		assert(false && "ERROR: An attempt was made to add a virtual texture page to a GlobalTexture texel after it was already transferred to a different texel!");
		std::unreachable();

		return std::optional<VirtualTexturePageTransferState>{};
	}

	auto VirtualTexturePageTransferState::OnPageRemovedFromGlobalTexture(GlobalTexturePageInfo&& pageInfo)
	{
		// If a virtual texture page was used in a transfer and is then removed from its destination
		// GlobalTexture page, then it enters the pending removal state. Logically, the page data
		// can still be found in the original GlobalTexture at mSrcPageInfo, so that is the source
		// page we provide when entering that state; if another transfer needs to happen, then
		// mSrcPageInfo will correctly be the source GlobalTexture page for that transfer.
		return std::optional<VirtualTexturePagePendingRemovalState>{ std::in_place, std::move(mSrcPageInfo) };
	}

	auto VirtualTexturePageTransferState::OnContextFinalization()
	{
		// If both mDestPageInfo and mSrcPageInfo refer to the same page, then we revert back to the
		// neutral state for this virtual texture page, since essentially no changes are being made to
		// any of the GlobalTextures. This case is discussed further in 
		// VirtualTexturePagePendingRemovalState::OnPageAddedToGlobalTexture().
		if (mDestPageInfo.ArePagesEquivalent(mSrcPageInfo))
			return std::optional<VirtualTexturePageNeutralState>{ std::in_place };
		
		return std::optional<VirtualTexturePageNeutralState>{};
	}

	auto VirtualTexturePageTransferState::GetOperationDetails(const VirtualTextureLogicalPage& logicalPage)
	{
		return std::make_unique<GlobalTexturePageTransferRequest>(std::move(mDestPageInfo), std::move(mSrcPageInfo));
	}
}
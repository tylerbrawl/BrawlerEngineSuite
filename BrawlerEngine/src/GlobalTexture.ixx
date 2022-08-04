module;
#include <array>
#include <vector>
#include <span>
#include <memory>
#include <algorithm>
#include <optional>
#include <cassert>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.GlobalTexture;
import Brawler.GlobalTexturePageReservationResults;
import Brawler.GlobalTextureFormatInfo;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.BindlessSRVAllocation;
import Brawler.D3D12.Texture2DBuilders;
import Brawler.GlobalTextureReservedPage;
import Brawler.GlobalTexturePageSwapOperation;
import Brawler.VirtualTextureLogicalPage;
import Util.General;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureMetadata;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.GPUSceneManager;
import Brawler.GPUSceneBuffer;
import Brawler.GPUSceneBufferID;
import Brawler.GPUSceneBufferUpdater;

export namespace Brawler
{
	template <DXGI_FORMAT Format>
	class GlobalTexture
	{
	private:
		static constexpr D3D12::Texture2DBuilder TEXTURE_2D_BUILDER{ Brawler::CreateGlobalTextureBuilder<Format>() };
		static constexpr std::size_t NUM_PAGES_PER_GLOBAL_TEXTURE_ROW = (TEXTURE_2D_BUILDER.GetResourceDescription().Width / GlobalTextureFormatInfo<Format>::PADDED_PAGE_DIMENSIONS);

		static constexpr std::size_t NUM_PAGES_IN_GLOBAL_TEXTURE = (NUM_PAGES_PER_GLOBAL_TEXTURE_ROW * NUM_PAGES_PER_GLOBAL_TEXTURE_ROW);

	public:
		GlobalTexture();

		GlobalTexture(const GlobalTexture& rhs) = delete;
		GlobalTexture& operator=(const GlobalTexture& rhs) = delete;

		GlobalTexture(GlobalTexture&& rhs) noexcept = default;
		GlobalTexture& operator=(GlobalTexture&& rhs) noexcept = default;
		
		/// <summary>
		/// Attempts to swap out virtual texture pages within this GlobalTexture instance for the
		/// pages referred to by the VirtualTextureLogicalPage instances described by logicalPageSpan.
		/// The returned struct describes the page swaps which were successful via its
		/// ActivePageSwapArr field.
		/// 
		/// Pages are allocated within the GlobalTexture instance in the order specified by
		/// logicalPageSpan; that is, a page which appears earlier in logicalPageSpan will be assigned
		/// a page in the GlobalTexture before one which appears later.
		/// 
		/// It is *NOT* the case that every page specified in logicalPageSpan will be given a location
		/// within the GlobalTexture after calling this function. A virtual texture page may fail
		/// to be assigned a place in the GlobalTexture if it is already full. The actual algorithm for
		/// determining page allocation is as follows:
		/// 
		/// For each page in the GlobalTexture, determine whether or not it already has an allocation.
		/// If it does not, then a page specified in logicalPageSpan is assigned to it; otherwise, if
		/// it is not being used by a combined page, then it is pushed to an array of possible candidates
		/// for replacement.
		/// 
		/// After all of the pages in the GlobalTexture have been searched in this manner, if there still
		/// exists at least one page in logicalPageSpan which was not assigned a page in the GlobalTexture,
		/// then the generated possible candidates array is sorted from least recently used to most recently
		/// used. The requested pages are then assigned these candidates. If no candidates remain and there
		/// are still pages left in logicalPageSpan which were not assigned a page in the GlobalTexture,
		/// then the function fails.
		/// 
		/// It is implied in the description of the algorithm that combined pages are never replaced in
		/// the GlobalTexture; this is to ensure that each virtual texture has at least one page which
		/// shaders can sample from. One might wonder, then, how these pages get removed. Upon the deletion
		/// of the VirtualTexture instance which corresponds to the virtual texture represented by the
		/// combined page, the allocation within the GlobalTexture for said page will be revoked. This is
		/// the only legal way to replace a combined page within the GlobalTexture.
		/// 
		/// *WARNING*: This function is *NOT* thread safe, since the thread-safe implementation would
		/// require an insane number of atomic operations. For the best performance, this function should
		/// be called asynchronously and concurrently alongside the rest of the Brawler Engine.
		/// </summary>
		/// <param name="logicalPageSpan">
		/// - A std::span&lt;const VirtualTextureLogicalPage&gt; which describes the pages which are to be
		///   assigned a page in the GlobalTexture.
		/// </param>
		/// <returns>
		/// The returned GlobalTexturePageReservationResults contains two fields.
		/// 
		/// GlobalTexturePageReservationResults::ActivePageSwapArr contains one GlobalTexturePageSwapOperation for
		/// each page in logicalPageSpan which was successfully given a location in the GlobalTexture.
		/// The ith entry in the array corresponds to the ith entry in logicalPageSpan. The actual
		/// GlobalTexturePageSwapOperation instances should be given to the GlobalTextureUploadBuffer
		/// in order to begin actually replacing the GlobalTexture data on the GPU side.
		/// 
		/// GlobalTexturePageReservationResults::HResult is an HRESULT value which describes the status of the 
		/// operation. The following values are possible:
		/// 
		///   - S_OK: The operation completed successfully (that is, every page in logicalPageSpan was
		///     assigned a location in the GlobalTexture).
		/// 
		///   - E_NOT_SUFFICIENT_BUFFER: At least one page within logicalPageSpan could not be assigned
		///     a location in the GlobalTexture. Keep in mind that ActivePageSwapArr may not be empty
		///     even if this value is returned.
		/// </returns>
		GlobalTexturePageReservationResults BeginGlobalTexturePageSwaps(const std::span<const VirtualTextureLogicalPage> logicalPageSpan);

	private:
		DirectX::XMUINT2 GetUnflattenedCoordinates(const std::uint32_t flattenedCoordinates) const;
		std::uint8_t GetGlobalTextureDescriptionBufferIndex() const;

	private:
		std::unique_ptr<D3D12::Texture2D> mTexture2DPtr;
		D3D12::BindlessSRVAllocation mGlobalTextureBindlessAllocation;
		std::array<GlobalTextureReservedPage, NUM_PAGES_IN_GLOBAL_TEXTURE> mReservedPageArr;
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
		mReservedPageArr(),
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
	GlobalTexturePageReservationResults GlobalTexture<Format>::BeginGlobalTexturePageSwaps(const std::span<const VirtualTextureLogicalPage> logicalPageSpan)
	{
		assert(mTexture2DPtr != nullptr);
		
		if constexpr (Util::General::IsDebugModeEnabled())
		{
			for (const auto& logicalPage : logicalPageSpan)
			{
				assert(logicalPage.VirtualTexturePtr != nullptr && "ERROR: A GlobalTexture instance was provided a nullptr VirtualTexture!");
				assert(logicalPage.VirtualTexturePtr->GetVirtualTextureMetadata().GetTextureFormat() == Format && "ERROR: A VirtualTexture instance was provided to a GlobalTexture instance with a different format!");
			}
		}

		if (logicalPageSpan.empty()) [[unlikely]]
			return GlobalTexturePageReservationResults{
				.ActivePageSwapArr{},
				.HResult = S_OK
			};

		GlobalTexturePageReservationResults reservationResults{};
		reservationResults.ActivePageSwapArr.reserve(logicalPageSpan.size());

		struct CandidatePage
		{
			GlobalTextureReservedPage* CandidatePagePtr;
			std::uint32_t FlattenedPageCoordinates;
		};

		std::vector<CandidatePage> candidatePageArr{};
		std::size_t currLogicalPageIndex = 0;
		
		{
			std::uint32_t currFlattenedPageCoordinates = 0;

			for (auto& reservedPage : mReservedPageArr)
			{
				// If a page in the GlobalTexture does not currently have an allocation, then it is a suitable
				// replacement.
				if (!reservedPage.HasAllocation())
				{
					reservedPage.SetVirtualTexturePage(logicalPageSpan[currLogicalPageIndex++]);

					const DirectX::XMUINT2 unflattenedCoordinates{ GetUnflattenedCoordinates(currFlattenedPageCoordinates) };
					reservationResults.ActivePageSwapArr.push_back(std::make_unique<GlobalTexturePageSwapOperation>(GlobalTexturePageSwapOperation::InitInfo{
						.GlobalTexturePtr = mTexture2DPtr.get(),
						.PageDimensions = GlobalTextureFormatInfo<Format>::PADDED_PAGE_DIMENSIONS,
						.NewReservedPage{ reservedPage },
						.OldReservedPage{},
						.StorageReservedPagePtr = &reservedPage,
						.GlobalTextureXPageCoordinates = static_cast<std::uint8_t>(unflattenedCoordinates.x),
						.GlobalTextureYPageCoordinates = static_cast<std::uint8_t>(unflattenedCoordinates.y),
						.GlobalTextureDescriptionBufferIndex = GetGlobalTextureDescriptionBufferIndex()
					}));

					// Exit early if that was the last page which needed to be assigned.
					if (currLogicalPageIndex == logicalPageSpan.size())
					{
						reservationResults.HResult = S_OK;
						return reservationResults;
					}
				}

				// Otherwise, if the page is not a combined page, then it is a suitable candidate page.
				else if (reservedPage.GetAllocatedPageType() != VirtualTexturePageType::COMBINED_PAGE)
					candidatePageArr.push_back(CandidatePage{
						.CandidatePagePtr = &reservedPage,
						.FlattenedPageCoordinates = currFlattenedPageCoordinates
					});

				++currFlattenedPageCoordinates;
			}
		}

		assert(currLogicalPageIndex < logicalPageSpan.size());

		// Sort the candidates based on when they were last used.
		std::ranges::sort(candidatePageArr, [] (const CandidatePage& lhs, const CandidatePage& rhs)
		{
			return (lhs.CandidatePagePtr->GetLastUsedFrameNumber() < rhs.CandidatePagePtr->GetLastUsedFrameNumber());
		});

		// Now, assign each page a candidate page in the order specified by logicalPageSpan.
		for (const auto& candidatePage : candidatePageArr)
		{
			GlobalTextureReservedPage oldPageCopy{ *(candidatePage.CandidatePagePtr) };

			candidatePage.CandidatePagePtr->SetVirtualTexturePage(logicalPageSpan[currLogicalPageIndex++]);

			const DirectX::XMUINT2 unflattenedCoordinates{ GetUnflattenedCoordinates(candidatePage.FlattenedPageCoordinates) };
			reservationResults.ActivePageSwapArr.push_back(std::make_unique<GlobalTexturePageSwapOperation>(GlobalTexturePageSwapOperation::InitInfo{
				.GlobalTexturePtr = mTexture2DPtr.get(),
				.PageDimensions = GlobalTextureFormatInfo<Format>::PADDED_PAGE_DIMENSIONS,
				.NewReservedPage{ *(candidatePage.CandidatePagePtr) },
				.OldReservedPage{ std::move(oldPageCopy) },
				.StorageReservedPagePtr = candidatePage.CandidatePagePtr,
				.GlobalTextureXPageCoordinates = static_cast<std::uint8_t>(unflattenedCoordinates.x),
				.GlobalTextureYPageCoordinates = static_cast<std::uint8_t>(unflattenedCoordinates.y),
				.GlobalTextureDescriptionBufferIndex = GetGlobalTextureDescriptionBufferIndex()
			}));

			// Exit if that was the last page which needed to be assigned.
			if (currLogicalPageIndex == logicalPageSpan.size())
			{
				reservationResults.HResult = S_OK;
				return reservationResults;
			}
		}

		// If we get to this point, then we couldn't assign every page in logicalPageSpan a
		// page in the GlobalTexture.
		assert(currLogicalPageIndex < logicalPageSpan.size());

		reservationResults.HResult = E_NOT_SUFFICIENT_BUFFER;
		return reservationResults;
	}

	template <DXGI_FORMAT Format>
	DirectX::XMUINT2 GlobalTexture<Format>::GetUnflattenedCoordinates(const std::uint32_t flattenedCoordinates) const
	{
		return DirectX::XMUINT2{
			(flattenedCoordinates % NUM_PAGES_PER_GLOBAL_TEXTURE_ROW),
			(flattenedCoordinates / NUM_PAGES_PER_GLOBAL_TEXTURE_ROW)
		};
	}

	template <DXGI_FORMAT Format>
	std::uint8_t GlobalTexture<Format>::GetGlobalTextureDescriptionBufferIndex() const
	{
		return static_cast<std::uint8_t>(mGlobalTextureDescriptionBufferSubAllocation.GetOffsetFromBufferStart() / sizeof(GlobalTextureDescription));
	}
}
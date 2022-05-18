module;
#include <memory>
#include <vector>
#include <format>
#include <cassert>
#include <DirectXTex.h>

// Damn Windows header macros...
#pragma push_macro("AddJob")
#undef AddJob

module Brawler.BC7CompressionRenderModule;
import Brawler.JobGroup;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.Timer;
import Util.Win32;
import Util.General;

namespace Brawler
{
	void BC7CompressionRenderModule::BuildFrameGraph(D3D12::FrameGraphBuilder& builder)
	{
		HandlePendingCompressionRequests(builder);
		CheckActiveCompressorsForDeletion();
	}

	void BC7CompressionRenderModule::HandlePendingCompressionRequests(D3D12::FrameGraphBuilder& builder)
	{
		// The FrameGraphBuilder is *NOT* thread-safe, so we need to create all of the transient
		// resources on a single thread.

		mPendingCompressorPtrArr.ForEach([&builder] (const std::unique_ptr<BC7ImageCompressor>& compressorPtr)
		{
			compressorPtr->CreateTransientResources(builder);
		});

		// We also don't want to allow multiple threads to move RenderPassBundles into FrameGraphBuilder
		// instances. Thankfully, creating the RenderPassBundles does not require having access to
		// the FrameGraphBuilder, so we can create these on multiple threads.

		const std::size_t pendingCompressorCount = mPendingCompressorPtrArr.GetSize();

		Brawler::JobGroup renderPassBundleCreationGroup{};
		renderPassBundleCreationGroup.Reserve(pendingCompressorCount);

		using RenderPassBundleArray = std::vector<D3D12::RenderPassBundle>;

		std::vector<RenderPassBundleArray> renderPassBundleArrayArr{};
		renderPassBundleArrayArr.resize(pendingCompressorCount);

		// For each BC7ImageCompressor, get its std::vector<D3D12::RenderPassBundle> containing the RenderPasses
		// which will be sent to the GPU and store it in renderPassBundleArrayArr.
		
		std::size_t currIndex = 0;
		mPendingCompressorPtrArr.ForEach([&renderPassBundleArrayArr, &currIndex, &renderPassBundleCreationGroup] (const std::unique_ptr<BC7ImageCompressor>& compressorPtr)
		{
			RenderPassBundleArray& assignedRenderPassBundleArr{ renderPassBundleArrayArr[currIndex++] };

			renderPassBundleCreationGroup.AddJob([&assignedRenderPassBundleArr, &compressorPtr] ()
			{
				assignedRenderPassBundleArr = compressorPtr->GetImageCompressionRenderPassBundles();
			});
		});

		Brawler::Timer bundleCreationTimer{};
		bundleCreationTimer.Start();

		renderPassBundleCreationGroup.ExecuteJobs();

		bundleCreationTimer.Stop();

		Util::Win32::WriteFormattedConsoleMessage(std::format(L"All BC7ImageCompressor RenderPassBundle instances were created in {} milliseconds.", bundleCreationTimer.GetElapsedTimeInMilliseconds()));

		// Now, we need to add the RenderPassBundles to the FrameGraphBuilder. The order which we do this
		// indeed has a significant impact. Since renderPassBundleArrayArr is a multi-dimensional vector,
		// we can split up the two primary ways of adding the RenderPassBundles into two categories:
		//
		//   - Row-Major: Add RenderPassBundles by BC7ImageCompressor instance. That is, all of the
		//     RenderPassBundles from BC7ImageCompressor A are added before any of those from
		//     BC7ImageCompressor B are added. This maximizes resource aliasing potential, since transient
		//     resources need only live for that instance's RenderPasses on the GPU timeline, but can introduce
		//     additional pipeline stalls due to the additional fence synchronization.
		//
		//   - Column-Major: Add RenderPassBundles by their index within the inner std::vector. Let
		//     A, B, and C be the steps of the compression process. Then, with this insertion ordering,
		//     all of the RenderPass commands for A across all of the BC7ImageCompressor instances are
		//     added before any of those for B are added. This minimizes the amount of required fence
		//     synchronization, since the same command queue will be used to process many commands
		//     one after the other. However, it also results in greatly increased memory consumption,
		//     since transient resources must stay alive without being aliased for a much greater duration
		//     of time.
		//
		// For now, I am going to go with row-major insertion. This is because the texture compression
		// process already has the potential to consume a large amount of GPU memory, and the massive
		// savings in memory usage more than outweigh the costs of the required fence synchronization.
		// The latter is especially true when considering the fact that this program is a tool, and not
		// the actual game engine, and so performance is not as critical as it might need to be.
		//
		// Anyways, I just wanted to write this to demonstrate my thought process, as well as for future
		// reference.

		enum class RenderPassBundleInsertionMode
		{
			ROW_MAJOR,
			COLUMN_MAJOR
		};

		static constexpr RenderPassBundleInsertionMode BUNDLE_INSERTION_MODE = RenderPassBundleInsertionMode::ROW_MAJOR;

		if constexpr (BUNDLE_INSERTION_MODE == RenderPassBundleInsertionMode::ROW_MAJOR)
		{
			// Row-Major Order
			//
			//   + Significantly Reduced Memory Consumption
			//   - Increased Pipeline Stalls Due to Fence Synchronization
			//
			// Check the above wall of text for more details.
			
			for (auto& renderPassBundleArr : renderPassBundleArrayArr)
			{
				for (auto&& renderPassBundle : renderPassBundleArr)
					builder.AddRenderPassBundle(std::move(renderPassBundle));
			}
		}
		else
		{
			// Column-Major Order
			//
			//   + Reduced Pipeline Stalls Due to Fence Synchronization
			//   - Significantly Increased Memory Consumption
			//
			// Check the above wall of text for more details.

			static constexpr auto ADD_COLUMN_BUNDLES_LAMBDA = []<std::size_t ColumnIndex>(std::vector<RenderPassBundleArray>& renderPassBundleArrayArr, D3D12::FrameGraphBuilder& builder)
			{
				for (auto& renderPassBundleArr : renderPassBundleArrayArr)
				{
					assert(ColumnIndex < renderPassBundleArr.size());
					builder.AddRenderPassBundle(std::move(renderPassBundleArr[ColumnIndex]));
				}
			};

			// Each BC7ImageCompressor should have created three RenderPassBundle instances.
			ADD_COLUMN_BUNDLES_LAMBDA.operator()<0>(renderPassBundleArrayArr, builder);
			ADD_COLUMN_BUNDLES_LAMBDA.operator()<1>(renderPassBundleArrayArr, builder);
			ADD_COLUMN_BUNDLES_LAMBDA.operator()<2>(renderPassBundleArrayArr, builder);
		}

		// Finally, move all of the pending compressors to the active compressor array.
		mPendingCompressorPtrArr.ForEach([this] (std::unique_ptr<BC7ImageCompressor>& compressorPtr)
		{
			mActiveCompressorPtrArr.push_back(std::move(compressorPtr));
		});

		mPendingCompressorPtrArr.Clear();
	}

	void BC7CompressionRenderModule::CheckActiveCompressorsForDeletion()
	{
		std::vector<BC7ImageCompressor*> deleteableCompressorPtrArr{};

		for (const auto& compressorPtr : mActiveCompressorPtrArr)
		{
			if (compressorPtr->ReadyForDeletion()) [[unlikely]]
				deleteableCompressorPtrArr.push_back(compressorPtr.get());
		}

		std::erase_if(mActiveCompressorPtrArr, [&deleteableCompressorPtrArr] (const std::unique_ptr<BC7ImageCompressor>& compressorPtr) { return (std::ranges::find(deleteableCompressorPtrArr, compressorPtr.get()) != deleteableCompressorPtrArr.end()); });
	}
}

#pragma pop_macro("AddJob")
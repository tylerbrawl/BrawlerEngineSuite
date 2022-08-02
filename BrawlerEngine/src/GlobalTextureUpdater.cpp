module;
#include <vector>
#include <memory>
#include <span>
#include <unordered_set>
#include <DxDef.h>

module Brawler.VirtualTextureManagementSubModule;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.GlobalTexturePageSwapOperation;

namespace Brawler
{
	GlobalTextureUpdater::GlobalTextureUpdatePass_T GlobalTextureUpdater::CreateGlobalTextureUpdatesRenderPass(const std::span<const std::unique_ptr<GlobalTextureUploadBuffer>> uploadBufferSpan) const
	{
		std::vector<GlobalTextureUpdateInfo> updateInfoArr{};
		std::unordered_set<D3D12::Texture2D*> updatedGlobalTextureSet{};

		GlobalTextureUpdatePass_T globalTextureUpdatePass{};
		globalTextureUpdatePass.SetRenderPassName("Virtual Texture Management - Global Texture Updates Pass");

		for (const auto& uploadBufferPtr : uploadBufferSpan)
		{
			const std::span<const std::unique_ptr<GlobalTexturePageSwapOperation>> pageSwapOperationSpan{ uploadBufferPtr->GetPageSwapOperationSpan() };

			for (const auto& pageSwapOperationPtr : pageSwapOperationSpan)
			{
				D3D12::Texture2D& destinationGlobalTexture{ pageSwapOperationPtr->GetGlobalTexture() };
				
				// Add the destination global texture to updatedGlobalTextureSet.
				updatedGlobalTextureSet.insert(&destinationGlobalTexture);

				D3D12::TextureCopyRegion destinationCopyRegion{ destinationGlobalTexture.GetSubResource(0), pageSwapOperationPtr->GetGlobalTextureCopyRegionBox() };

				updateInfoArr.push_back(GlobalTextureUpdateInfo{
					.DestinationCopyRegion{ std::move(destinationCopyRegion) },
					.SourceCopyBufferSnapshot{ pageSwapOperationPtr->GetGlobalTextureCopySubAllocation() }
				});
			}

			globalTextureUpdatePass.AddResourceDependency(D3D12::FrameGraphResourceDependency{
				.ResourcePtr = &(uploadBufferPtr->GetUploadBufferResource()),
				.RequiredState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE,
				.SubResourceIndex = 0
			});
		}

		for (const auto globalTexturePtr : updatedGlobalTextureSet)
		{
			D3D12::Texture2DSubResource globalTextureSubResource{ globalTexturePtr->GetSubResource(0) };
			globalTextureUpdatePass.AddResourceDependency(globalTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
		}

		globalTextureUpdatePass.SetInputData(GlobalTextureUpdatePassInfo{
			.UpdateInfoArr{ std::move(updateInfoArr) }
		});

		globalTextureUpdatePass.SetRenderPassCommands([] (D3D12::DirectContext& context, const GlobalTextureUpdatePassInfo& passInfo)
		{
			for (const auto& updateInfo : passInfo.UpdateInfoArr)
				context.CopyBufferToTexture(updateInfo.DestinationCopyRegion, updateInfo.SourceCopyBufferSnapshot);
		});

		return globalTextureUpdatePass;
	}
}
module;
#include <vector>
#include <optional>
#include <span>
#include <cassert>
#include "DxDef.h"

module Brawler.ResourceDescriptorTable;
import Brawler.GPUResourceHandle;
import Util.Engine;
import Brawler.Renderer;
import Brawler.ResourceDescriptorHeap;

namespace Brawler
{
	ResourceDescriptorTable::ResourceDescriptorTable() :
		mRangeArr(),
		mGPUVisibleDescriptorHandleInfo()
	{}

	void ResourceDescriptorTable::CreateCBV(const GPUResourceHandle& hResource)
	{
		AddDescriptor(hResource, ResourceDescriptorType::CBV);
	}

	void ResourceDescriptorTable::CreateSRV(const GPUResourceHandle& hResource)
	{
		AddDescriptor(hResource, ResourceDescriptorType::SRV);
	}

	void ResourceDescriptorTable::CreateUAV(const GPUResourceHandle& hResource)
	{
		AddDescriptor(hResource, ResourceDescriptorType::UAV);
	}

	void ResourceDescriptorTable::CreatePerFrameDescriptorTable()
	{
		if (!mGPUVisibleDescriptorHandleInfo.has_value()) [[likely]]
			Util::Engine::GetRenderer().GetResourceDescriptorHeap().CreatePerFrameDescriptorTable(*this);
	}

	const DescriptorHandleInfo& ResourceDescriptorTable::GetDescriptorHandlesForTableStart() const
	{
		assert(mGPUVisibleDescriptorHandleInfo.has_value() && "ERROR: An attempt was made to get the descriptor handles for the start of a ResourceDescriptorTable before the appropriate descriptors could be copied into the ResourceDescriptorHeap!");
		return *mGPUVisibleDescriptorHandleInfo;
	}

	void ResourceDescriptorTable::AddDescriptor(const GPUResourceHandle& hResource, const ResourceDescriptorType type)
	{
		assert(!mGPUVisibleDescriptorHandleInfo.has_value() && "ERROR: An attempt was made to add a descriptor to a ResourceDescriptorTable after it was already allocated in the ResourceDescriptorHeap! (This is an error for performance and memory efficiency reasons. If this attempt was intentional, try to add all of the required descriptors to the ResourceDescriptorTable at once before binding it to a root parameter binder.)");
		
		if (mRangeArr.empty() || mRangeArr.back().RootParamType != type)
		{
			DescriptorRange range{
				.RootParamType = type,
				.ResourcePtrArr{}
			};
			mRangeArr.push_back(std::move(range));
		}

		mRangeArr.back().ResourcePtrArr.push_back(&(*hResource));
	}

	std::span<const ResourceDescriptorTable::DescriptorRange> ResourceDescriptorTable::GetDescriptorRanges() const
	{
		return mRangeArr;
	}

	void ResourceDescriptorTable::SetDescriptorHandlesForTableStart(DescriptorHandleInfo&& descriptorInfo)
	{
		mGPUVisibleDescriptorHandleInfo = std::move(descriptorInfo);
	}
}
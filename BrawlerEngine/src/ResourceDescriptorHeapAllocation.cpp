module;
#include <optional>
#include <cassert>
#include <thread>

module Brawler.ResourceDescriptorHeapAllocation;
import Util.Engine;
import Brawler.ResourceDescriptorHeap;
import Brawler.Renderer;

namespace Brawler
{
	ResourceDescriptorHeapAllocation::ResourceDescriptorHeapAllocation(const std::uint32_t heapIndex) :
		mHeapIndex(heapIndex),
		mHandleInfo()
	{}

	ResourceDescriptorHeapAllocation::~ResourceDescriptorHeapAllocation()
	{
		DeleteAllocation();
	}

	ResourceDescriptorHeapAllocation::ResourceDescriptorHeapAllocation(ResourceDescriptorHeapAllocation&& rhs) noexcept :
		mHeapIndex(std::move(rhs.mHeapIndex)),
		mHandleInfo(std::move(rhs.mHandleInfo))
	{
		rhs.mHeapIndex.reset();
	}

	ResourceDescriptorHeapAllocation& ResourceDescriptorHeapAllocation::operator=(ResourceDescriptorHeapAllocation&& rhs) noexcept
	{
		DeleteAllocation();
		mHeapIndex = std::move(rhs.mHeapIndex);
		rhs.mHeapIndex.reset();

		mHandleInfo = std::move(rhs.mHandleInfo);

		return *this;
	}

	bool ResourceDescriptorHeapAllocation::IsAllocationValid() const
	{
		return mHeapIndex.has_value();
	}

	const DescriptorHandleInfo& ResourceDescriptorHeapAllocation::GetDescriptorHandles() const
	{
		assert(IsAllocationValid() && "ERROR: An attempt was made to call ResourceDescriptorHeapAllocation::GetDescriptorHandles() for an invalid allocation!");

		return mHandleInfo;
	}

	void ResourceDescriptorHeapAllocation::DeleteAllocation()
	{
		Util::Engine::GetRenderer().GetResourceDescriptorHeap().DeleteAllocation(std::move(*this));
	}
}
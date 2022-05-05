module;
#include <vector>
#include <optional>
#include <span>
#include "DxDef.h"

export module Brawler.ResourceDescriptorTable;
import Brawler.ResourceDescriptorType;
import Brawler.DescriptorHandleInfo;

export namespace Brawler
{
	class I_GPUResource;
	class GPUResourceHandle;
	class ResourceDescriptorHeap;
}

export namespace Brawler
{
	class ResourceDescriptorTable
	{
	private:
		friend class ResourceDescriptorHeap;

	private:
		struct DescriptorRange
		{
			ResourceDescriptorType RootParamType;
			std::vector<const I_GPUResource*> ResourcePtrArr;
		};

	public:
		ResourceDescriptorTable();

		void CreateCBV(const GPUResourceHandle& hResource);
		void CreateSRV(const GPUResourceHandle& hResource);
		void CreateUAV(const GPUResourceHandle& hResource);

		void CreatePerFrameDescriptorTable();
		const DescriptorHandleInfo& GetDescriptorHandlesForTableStart() const;

	private:
		void AddDescriptor(const GPUResourceHandle& hResource, const ResourceDescriptorType type);
		std::span<const DescriptorRange> GetDescriptorRanges() const;
		void SetDescriptorHandlesForTableStart(DescriptorHandleInfo&& descriptorInfo);

	private:
		std::vector<DescriptorRange> mRangeArr;
		std::optional<DescriptorHandleInfo> mGPUVisibleDescriptorHandleInfo;
	};
}
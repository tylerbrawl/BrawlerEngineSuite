module;
#include "DxDef.h"

export module Brawler.D3D12.GPUSplitBarrierToken;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUSplitBarrierToken
		{
		public:
			struct BarrierInfo
			{
				I_GPUResource* ResourcePtr;
				D3D12_RESOURCE_STATES DesiredState;
			};

		public:
			GPUSplitBarrierToken() = default;
			GPUSplitBarrierToken(I_GPUResource& resource, const D3D12_RESOURCE_STATES desiredState);

			GPUSplitBarrierToken(const GPUSplitBarrierToken& rhs) = delete;
			GPUSplitBarrierToken& operator=(const GPUSplitBarrierToken& rhs) = delete;

			GPUSplitBarrierToken(GPUSplitBarrierToken&& rhs) noexcept = default;
			GPUSplitBarrierToken& operator=(GPUSplitBarrierToken&& rhs) noexcept = default;

			const BarrierInfo& GetBarrierInfo() const;

		private:
			BarrierInfo mBarrierInfo;
		};
	}
}
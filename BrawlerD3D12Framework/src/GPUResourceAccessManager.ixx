module;
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceAccessManager;
import Brawler.D3D12.FrameGraphResourceDependency;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceAccessManager
		{
		public:
			GPUResourceAccessManager() = default;

			void SetCurrentResourceDependencies(const std::span<const FrameGraphResourceDependency> dependencySpan);
			void ClearCurrentResourceDependencies();

			bool IsResourceAccessValid(const I_GPUResource& resource, const D3D12_RESOURCE_STATES requiredState) const;

		private:
#ifdef _DEBUG
			std::span<const FrameGraphResourceDependency> mCurrDependencySpan;
#endif // _DEBUG
		};
	}
}
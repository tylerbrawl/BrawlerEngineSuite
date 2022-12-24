module;
#include <functional>

export module Brawler.D3D12.CopyContext;
import Brawler.D3D12.GPUCommandContext;
import Brawler.D3D12.GPUCommandQueueType;

export namespace Brawler
{
	namespace D3D12
	{
		class CopyContext final : public GPUCommandContext<CopyContext, GPUCommandQueueType::COPY>
		{
		private:
			friend GPUCommandContext<CopyContext, GPUCommandQueueType::COPY>;

		public:
			CopyContext() = default;

			CopyContext(const CopyContext& rhs) = delete;
			CopyContext& operator=(const CopyContext& rhs) = delete;

			CopyContext(CopyContext&& rhs) noexcept = default;
			CopyContext& operator=(CopyContext&& rhs) noexcept = default;

		private:
			void RecordCommandListIMPL(const std::function<void(CopyContext&)>& recordJob);
			void PrepareCommandListIMPL();
		};
	}
}
module;
#include <functional>

export module Brawler.D3D12.CopyContext;
import Brawler.D3D12.GPUCommandContext;
import Brawler.D3D12.GPUCommandQueueType;

export namespace Brawler
{
	namespace D3D12
	{
		class CopyContext final : public GPUCommandContext<GPUCommandQueueType::COPY>
		{
		public:
			CopyContext() = default;

			CopyContext(const CopyContext& rhs) = delete;
			CopyContext& operator=(const CopyContext& rhs) = delete;

			CopyContext(CopyContext&& rhs) noexcept = default;
			CopyContext& operator=(CopyContext&& rhs) noexcept = default;

			void RecordCommandListIMPL(const std::function<void(CopyContext&)>& recordJob) override;
		};
	}
}
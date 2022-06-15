module;
#include <span>
#include <string_view>

export module Brawler.D3D12.I_RenderPass;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUCommandQueueContextType;
import Brawler.D3D12.FrameGraphResourceDependency;

export namespace Brawler
{
	namespace D3D12
	{
		/// <summary>
		/// This class is used as the base of RenderPass<InputDataType>. It is needed for the
		/// sake of type erasure.
		/// </summary>
		template <GPUCommandQueueType QueueType>
		class I_RenderPass
		{
		protected:
			I_RenderPass() = default;

		public:
			virtual ~I_RenderPass() = default;

			I_RenderPass(const I_RenderPass& rhs) = delete;
			I_RenderPass& operator=(const I_RenderPass& rhs) = delete;

			I_RenderPass(I_RenderPass&& rhs) noexcept = default;
			I_RenderPass& operator=(I_RenderPass&& rhs) noexcept = default;

			virtual bool RecordRenderPassCommands(GPUCommandQueueContextType<QueueType>& context) const = 0;
			virtual std::span<const FrameGraphResourceDependency> GetResourceDependencies() const = 0;
			virtual const std::string_view GetRenderPassName() const = 0;
		};
	}
}
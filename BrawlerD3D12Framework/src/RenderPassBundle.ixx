module;
#include <vector>
#include <memory>
#include <span>

export module Brawler.D3D12.RenderPassBundle;
import Brawler.D3D12.RenderPass;
import Brawler.D3D12.I_RenderPass;
import Brawler.SortedVector;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.CompositeEnum;

export namespace Brawler
{
	namespace D3D12
	{
		class TransientGPUResourceAliasTracker;
		class I_GPUResource;
	}
}

namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphBuilder;
		class GPUExecutionModule;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		/// <summary>
		/// A RenderPassBundle contains a set of render passes which are to be submitted
		/// simultaneously to the GPU. This allows one to manually pair specific pairs of
		/// direct and compute queue workloads (e.g., for asynchronous compute).
		/// 
		/// For instance, one can pair shadow mapping on the direct queue with light culling
		/// on the compute queue by adding them both to the same RenderPassBundle instance.
		/// </summary>
		class RenderPassBundle
		{
		private:
			friend class FrameGraphBuilder;
			friend class GPUExecutionModule;

		public:
			RenderPassBundle() = default;

			RenderPassBundle(const RenderPassBundle& rhs) = delete;
			RenderPassBundle& operator=(const RenderPassBundle& rhs) = delete;

			RenderPassBundle(RenderPassBundle&& rhs) noexcept = default;
			RenderPassBundle& operator=(RenderPassBundle&& rhs) noexcept = default;

			template <typename InputDataType>
			void AddDirectRenderPass(RenderPass<GPUCommandQueueType::DIRECT, InputDataType>&& directPass);

			template <typename InputDataType>
			void AddComputeRenderPass(RenderPass<GPUCommandQueueType::COMPUTE, InputDataType>&& computePass);

			template <typename InputDataType>
			void AddCopyRenderPass(RenderPass<GPUCommandQueueType::COPY, InputDataType>&& copyPass);

			template <GPUCommandQueueType QueueType>
				requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
			std::size_t GetRenderPassCount() const;

			/// <summary>
			/// Retrieves the total number of RenderPass instances across all of the
			/// queues which are used in this RenderPassBundle instance.
			/// </summary>
			/// <returns>
			/// The function returns the total number of RenderPass instances across
			/// all of the queues which are used in this RenderPassBundle instance.
			/// </returns>
			std::size_t GetTotalRenderPassCount() const;

			template <GPUCommandQueueType QueueType>
				requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
			std::span<std::unique_ptr<I_RenderPass<QueueType>>> GetRenderPassSpan();

			template <GPUCommandQueueType QueueType>
				requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
			std::span<const std::unique_ptr<I_RenderPass<QueueType>>> GetRenderPassSpan() const;

			CompositeEnum<GPUCommandQueueType> GetUsedQueues() const;

			std::span<I_GPUResource* const> GetResourceDependencies() const;

		private:
			template <GPUCommandQueueType PassQueueType, typename InputDataType>
			void AddResourceDependenciesForRenderPass(const RenderPass<PassQueueType, InputDataType>& renderPass);

			void UpdateTransientGPUResourceAliasTracker(TransientGPUResourceAliasTracker& aliasTracker) const;

			void MarkAsSyncPoint();

			void SetRenderPassBundleID(const std::uint32_t bundleID);
			std::uint32_t GetRenderPassBundleID() const;

		public:
			bool IsSyncPoint() const;
			
		private:
			std::vector<std::unique_ptr<I_RenderPass<GPUCommandQueueType::DIRECT>>> mDirectPassArr;
			std::vector<std::unique_ptr<I_RenderPass<GPUCommandQueueType::COMPUTE>>> mComputePassArr;
			std::vector<std::unique_ptr<I_RenderPass<GPUCommandQueueType::COPY>>> mCopyPassArr;
			Brawler::SortedVector<I_GPUResource*> mResourceDependencyArr;
			std::uint32_t mBundleID;
			bool mIsSyncPoint;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename InputDataType>
		void RenderPassBundle::AddDirectRenderPass(RenderPass<GPUCommandQueueType::DIRECT, InputDataType>&& directPass)
		{
			AddResourceDependenciesForRenderPass(directPass);
			
			mDirectPassArr.push_back(std::make_unique<RenderPass<GPUCommandQueueType::DIRECT, InputDataType>>(std::move(directPass)));
		}

		template <typename InputDataType>
		void RenderPassBundle::AddComputeRenderPass(RenderPass<GPUCommandQueueType::COMPUTE, InputDataType>&& computePass)
		{
			AddResourceDependenciesForRenderPass(computePass);
			
			mComputePassArr.push_back(std::make_unique<RenderPass<GPUCommandQueueType::COMPUTE, InputDataType>>(std::move(computePass)));
		}

		template <typename InputDataType>
		void RenderPassBundle::AddCopyRenderPass(RenderPass<GPUCommandQueueType::COPY, InputDataType>&& copyPass)
		{
			AddResourceDependenciesForRenderPass(copyPass);
			
			mCopyPassArr.push_back(std::make_unique<RenderPass<GPUCommandQueueType::COPY, InputDataType>>(std::move(copyPass)));
		}

		template <GPUCommandQueueType QueueType>
			requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
		std::size_t RenderPassBundle::GetRenderPassCount() const
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return mDirectPassArr.size();

			if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return mComputePassArr.size();

			if constexpr (QueueType == GPUCommandQueueType::COPY)
				return mCopyPassArr.size();
		}

		template <GPUCommandQueueType QueueType>
			requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
		std::span<std::unique_ptr<I_RenderPass<QueueType>>> RenderPassBundle::GetRenderPassSpan()
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return std::span<std::unique_ptr<I_RenderPass<GPUCommandQueueType::DIRECT>>>{ mDirectPassArr };

			if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return std::span<std::unique_ptr<I_RenderPass<GPUCommandQueueType::COMPUTE>>>{ mComputePassArr };

			if constexpr (QueueType == GPUCommandQueueType::COPY)
				return std::span<std::unique_ptr<I_RenderPass<GPUCommandQueueType::COPY>>>{ mCopyPassArr };
		}

		template <GPUCommandQueueType QueueType>
			requires (QueueType != GPUCommandQueueType::COUNT_OR_ERROR)
		std::span<const std::unique_ptr<I_RenderPass<QueueType>>> RenderPassBundle::GetRenderPassSpan() const
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
				return std::span<const std::unique_ptr<I_RenderPass<GPUCommandQueueType::DIRECT>>>{ mDirectPassArr };

			if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
				return std::span<const std::unique_ptr<I_RenderPass<GPUCommandQueueType::COMPUTE>>>{ mComputePassArr };

			if constexpr (QueueType == GPUCommandQueueType::COPY)
				return std::span<const std::unique_ptr<I_RenderPass<GPUCommandQueueType::COPY>>>{ mCopyPassArr };
		}

		template <GPUCommandQueueType PassQueueType, typename InputDataType>
		void RenderPassBundle::AddResourceDependenciesForRenderPass(const RenderPass<PassQueueType, InputDataType>& renderPass)
		{
			const std::span<const FrameGraphResourceDependency> resourceDependencySpan{ renderPass.GetResourceDependencies() };
			mResourceDependencyArr.Reserve(mResourceDependencyArr.GetSize() + resourceDependencySpan.size());

			for (const auto& dependency : resourceDependencySpan)
				mResourceDependencyArr.Insert(dependency.ResourcePtr);
		}
	}
}
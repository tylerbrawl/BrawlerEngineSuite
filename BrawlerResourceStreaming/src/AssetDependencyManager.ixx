module;
#include <vector>
#include <mutex>
#include <memory>
#include <array>

export module Brawler.AssetDependencyManager;
import Brawler.AssetDependencyGroup;
import Brawler.PersistentAssetDependencyGroup;
import Brawler.AssetDataRequests;
import Brawler.JobPriority;

export namespace Brawler
{
	class AssetManager;
}

namespace IMPL
{
	template <typename T>
	struct AssetDataRequestPullContext
	{
		std::vector<std::unique_ptr<T>> RequestArr;
		std::size_t BytesHandledInRequests;
		std::size_t MaxDesiredBytesAllowed;
	};

	class AssetDataRequestHandler
	{
	public:
		AssetDataRequestHandler();

		AssetDataRequestHandler(const AssetDataRequestHandler& rhs) = delete;
		AssetDataRequestHandler& operator=(const AssetDataRequestHandler& rhs) = delete;

		AssetDataRequestHandler(AssetDataRequestHandler&& rhs) noexcept = default;
		AssetDataRequestHandler& operator=(AssetDataRequestHandler&& rhs) noexcept = default;

		void SubmitAssetDataLoadRequest(std::unique_ptr<Brawler::AssetDataLoadRequest>&& loadRequest);
		void SubmitAssetDataUnloadRequest(std::unique_ptr<Brawler::AssetDataUnloadRequest>&& unloadRequest);

		void SortAssetDataRequestQueues();

		template <typename T>
		void PullAssetDataRequests(AssetDataRequestPullContext<T>& pullContext);

		bool IsLoadRequestQueueEmpty() const;
		bool IsUnloadRequestQueueEmpty() const;

	private:
		std::vector<std::unique_ptr<Brawler::AssetDataLoadRequest>> mLoadRequestQueue;
		mutable std::mutex mLoadRequestQueueCritSection;

		std::vector<std::unique_ptr<Brawler::AssetDataUnloadRequest>> mUnloadRequestQueue;
		mutable std::mutex mUnloadRequestQueueCritSection;
	};
}

export namespace Brawler
{
	class AssetDependencyManager
	{
	private:
		template <typename T>
		struct DependencyGroupForest
		{
			/// <summary>
			/// These are the currently tracked asset dependency groups in the DependencyGroupForest,
			/// i.e., they are the head nodes of the active asset dependency graphs/trees.
			/// 
			/// NOTE: It is expected that ActiveGroups is written to by ONLY one thread at a time.
			/// </summary>
			std::vector<std::unique_ptr<T>> ActiveGroups;

			/// <summary>
			/// These are the asset dependency groups which will be added to the ActiveGroups during
			/// the next update.
			/// </summary>
			std::vector<std::unique_ptr<T>> PendingGroups;

			mutable std::mutex PendingGroupsCritSection;
		};

	public:
		enum class OptimizationMode
		{
			/// <summary>
			/// Reduces the chances of stuttering by decreasing the amount of data which can
			/// be loaded or unloaded at once. However, this can lead to increased load/unload
			/// times.
			/// </summary>
			REDUCE_STUTTER,

			/// <summary>
			/// Reduces the time it takes to load or unload assets by increasing the amount of
			/// data which can be loaded or unloaded at once. However, this can lead to increased
			/// stuttering.
			/// </summary>
			REDUCE_LOAD_TIMES
		};

	public:
		explicit AssetDependencyManager(AssetManager& owningManager);

		AssetDependencyManager(const AssetDependencyManager& rhs) = delete;
		AssetDependencyManager& operator=(const AssetDependencyManager& rhs) = delete;

		AssetDependencyManager(AssetDependencyManager&& rhs) noexcept = default;
		AssetDependencyManager& operator=(AssetDependencyManager&& rhs) noexcept = default;

		void UpdateAssetDependencies();

		void SubmitAssetDependencyGroup(std::unique_ptr<AssetDependencyGroup>&& dependencyGroup);
		void SubmitAssetDependencyGroup(std::unique_ptr<PersistentAssetDependencyGroup>&& dependencyGroup);

		void SubmitAssetDataLoadRequest(std::unique_ptr<AssetDataLoadRequest>&& loadRequest, const JobPriority priority);
		void SubmitAssetDataUnloadRequest(std::unique_ptr<AssetDataUnloadRequest>&& unloadRequest, const JobPriority priority);

		void SetOptimizationMode(const OptimizationMode optimizationMode);

	private:
		void ActivatePendingAssetDependencies();
		void CheckDependenciesForAssetDataUpdateRequests();
		void ProcessAssetDataUpdateRequests();

		std::vector<std::unique_ptr<AssetDataLoadRequest>> GetLoadRequestsForSingleJob();
		std::vector<std::unique_ptr<AssetDataUnloadRequest>> GetUnloadRequestsForSingleJob();

	private:
		AssetManager* const mOwningManager;
		DependencyGroupForest<AssetDependencyGroup> mAssetDependencyGroups;
		DependencyGroupForest<PersistentAssetDependencyGroup> mPersistentDependencyGroups;
		std::array<IMPL::AssetDataRequestHandler, std::to_underlying(JobPriority::COUNT)> mRequestHandlerArr;
		OptimizationMode mOptimizationMode;
	};
}
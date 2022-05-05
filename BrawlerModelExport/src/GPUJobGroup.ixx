module;
#include <vector>
#include <functional>
#include <unordered_map>

export module Brawler.D3D12.GPUJobGroup;
import Brawler.D3D12.GPUResourceHandle;
import Brawler.D3D12.GPUResourceAccessMode;
import Brawler.D3D12.GPUEventHandle;

export namespace Brawler
{
	namespace D3D12
	{
		class GPUCommandManager;
		class DirectContext;
		class ComputeContext;
		class CopyContext;
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class GPUJobGroup
		{
			friend class GPUCommandManager;

		public:
			GPUJobGroup() = default;

			GPUJobGroup(const GPUJobGroup& rhs) = delete;
			GPUJobGroup& operator=(const GPUJobGroup& rhs) = delete;

			GPUJobGroup(GPUJobGroup&& rhs) noexcept = default;
			GPUJobGroup& operator=(GPUJobGroup&& rhs) noexcept = default;

			/// <summary>
			/// Adds a GPU job for recording commands to a DirectContext. The underlying
			/// command list will be submitted on the direct queue of the D3D12Device.
			/// </summary>
			/// <param name="job">
			/// - The job which will record commands into a command list for submission
			///   to the current D3D12Device's direct queue.
			/// </param>
			void AddDirectJob(std::function<void(DirectContext&)>&& job);

			/// <summary>
			/// Adds a GPU job for recording commands to a ComputeContext. The underlying
			/// command list will be submitted on the compute queue of the D3D12Device.
			/// </summary>
			/// <param name="job">
			/// - The job which will record commands into a command list for submission
			///   to the current D3D12Device's compute queue.
			/// </param>
			void AddComputeJob(std::function<void(ComputeContext&)>&& job);

			/// <summary>
			/// Adds a GPU job for recording commands to a CopyContext. The underlying
			/// command list will be submitted on the copy queue of the D3D12Device.
			/// </summary>
			/// <param name="job">
			/// - The job which will record commands into a command list for submission
			///   to the current D3D12Device's copy queue.
			/// </param>
			void AddCopyJob(std::function<void(CopyContext&)>&& job);

			GPUEventHandle ExecuteJobs();

			GPUResourceReadHandle CreateGPUResourceReadHandle(I_GPUResource& resource);
			GPUResourceWriteHandle CreateGPUResourceWriteHandle(I_GPUResource& resource);

		private:
			bool CanGPUResourceHandleBeCreated(const I_GPUResource& resource, const GPUResourceAccessMode desiredAccessMode) const;
			void RegisterCreatedGPUResourceHandle(const I_GPUResource& resource, const GPUResourceAccessMode desiredAccessMode);

			std::vector<std::function<void(DirectContext&)>> ExtractDirectJobs();
			std::vector<std::function<void(ComputeContext&)>> ExtractComputeJobs();
			std::vector<std::function<void(CopyContext&)>> ExtractCopyJobs();

		private:
			std::vector<std::function<void(DirectContext&)>> mDirectJobArr;
			std::vector<std::function<void(ComputeContext&)>> mComputeJobArr;
			std::vector<std::function<void(CopyContext&)>> mCopyJobArr;

#ifdef _DEBUG
			std::unordered_map<const I_GPUResource*, GPUResourceAccessMode> mAllowedResourceHandlesMap;
#endif // _DEBUG
		};
	}
}
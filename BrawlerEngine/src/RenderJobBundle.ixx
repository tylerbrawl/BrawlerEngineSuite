module;
#include <vector>
#include <span>

export module Brawler.RenderJobBundle;
import Brawler.RenderJobs;
import Brawler.RenderEventHandle;

namespace Brawler
{
	class RenderJobManager;
}

export namespace Brawler
{
	class RenderJobBundle
	{
	private:
		friend class RenderJobManager;

	public:
		RenderJobBundle();

		void AddGraphicsJob(GraphicsJob&& graphicsJob);
		std::span<GraphicsJob> GetGraphicsJobs();
		std::size_t GetGraphicsJobCount() const;

		void AddComputeJob(ComputeJob&& computeJob);
		std::span<ComputeJob> GetComputeJobs();
		std::size_t GetComputeJobCount() const;

		void AddCopyJob(CopyJob&& copyJob);
		std::span<CopyJob> GetCopyJobs();
		std::size_t GetCopyJobCount() const;

		RenderEventHandle SubmitRenderJobs();

		void InitializeResourceTransitionManagers();

	private:
		/// <summary>
		/// This function checks to make sure that all of the I_RenderJobs in this RenderJobBundle
		/// do not have conflicting resource dependencies. This can occur if one of the following
		/// conditions holds true:
		/// 
		///   - A resource which is not a buffer and does not have the 
		///		D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag is used in more than one queue.
		/// 
		///   - A resource dependency is specified with Brawler::ResourceAccessMode::WRITE privileges,
		///     but the resource specified by the dependency is accessed by more than one job within
		///     the same RenderJobBundle.
		/// 
		///   - A resource dependency is specified with Brawler::ResourceAccessMode::READ privileges,
		///     but the resource is used with privileges greater than Brawler::ResourceAccessMode::READ
		///     by another job within the same RenderJobBundle.
		/// 
		/// (NOTE: This check is only done in Debug builds.)
		/// </summary>
		/// <returns>
		/// This function returns true if the resource dependencies between the I_RenderJobs of this
		/// bundle do *NOT* conflict and false otherwise.
		/// </returns>
		bool CheckResourceDependencyConflicts() const;

	private:
		std::vector<GraphicsJob> mGraphicsJobArr;
		std::vector<ComputeJob> mComputeJobArr;
		std::vector<CopyJob> mCopyJobArr;
	};
}
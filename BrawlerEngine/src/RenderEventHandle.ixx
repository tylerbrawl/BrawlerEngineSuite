module;
#include <memory>
#include <atomic>

export module Brawler.RenderEventHandle;

namespace Brawler
{
	class RenderJobManager;
}

export namespace Brawler
{
	class RenderEventHandle
	{
	private:
		friend class RenderJobManager;

	private:
		explicit RenderEventHandle(const std::shared_ptr<std::atomic<bool>> flag);

	public:
		~RenderEventHandle() = default;

		RenderEventHandle(const RenderEventHandle& rhs) = default;
		RenderEventHandle& operator=(const RenderEventHandle& rhs) = default;

		RenderEventHandle(RenderEventHandle&& rhs) noexcept = default;
		RenderEventHandle& operator=(RenderEventHandle&& rhs) noexcept = default;

		/// <summary>
		/// Checks if the relevant I_RenderJobs have had their commands
		/// executed by the GPU.
		/// </summary>
		/// <returns>
		/// The function returns true if the GPU has successfully executed all
		/// of the relevant commands and false otherwise.
		/// </returns>
		bool IsEventCompleted() const;

		/// <summary>
		/// Blocks the calling thread until the relevant I_RenderJobs have had
		/// their commands executed by the GPU. Despite its name, this function
		/// does not do any busy waiting; instead, the calling thread will
		/// attempt to take CPU jobs from the worker thread queues.
		/// </summary>
		void WaitForEventCompletion() const;

	private:
		std::shared_ptr<std::atomic<bool>> mFlag;
	};
}
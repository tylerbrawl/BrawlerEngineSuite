module;
#include <functional>

export module Brawler.RenderJobs;
import Brawler.I_RenderJob;
import Brawler.I_RenderContext;
import Brawler.GraphicsContext;
import Brawler.ComputeContext;
import Brawler.CopyContext;

export namespace Brawler
{
	class RenderJobManager;
}

namespace
{
	template <typename ContextType>
		requires std::derived_from<ContextType, Brawler::I_RenderContext>
	class RenderJob final : public Brawler::I_RenderJob
	{
	private:
		friend class Brawler::RenderJobManager;

	public:
		RenderJob();

		/// <summary>
		/// Sets the function which is called by the RenderJobManager to record 
		/// commands into a render context.
		/// </summary>
		/// <param name="callback">
		/// - The function which will record commands into a render context.
		/// </param>
		void SetCallback(std::function<void(ContextType&)>&& callback);

		void RecordCommands(ContextType& context);

	private:
		std::function<void(ContextType&)> mCallback;
	};

	// --------------------------------------------------------------------------------------

	template <typename ContextType>
		requires std::derived_from<ContextType, Brawler::I_RenderContext>
	RenderJob<ContextType>::RenderJob() :
		mCallback()
	{}

	template <typename ContextType>
		requires std::derived_from<ContextType, Brawler::I_RenderContext>
	void RenderJob<ContextType>::SetCallback(std::function<void(ContextType&)>&& callback)
	{
		mCallback = std::move(callback);
	}

	template <typename ContextType>
		requires std::derived_from<ContextType, Brawler::I_RenderContext>
	void RenderJob<ContextType>::RecordCommands(ContextType& context)
	{
		context.SetRecordingJob(*this);
		
		mCallback(context);
	}
}

export namespace Brawler
{
	using GraphicsJob = RenderJob<GraphicsContext>;
	using ComputeJob = RenderJob<ComputeContext>;
	using CopyJob = RenderJob<CopyContext>;
}
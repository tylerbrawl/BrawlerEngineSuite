module;
#include <unordered_map>
#include "DxDef.h"

export module Brawler.I_RenderJob;
import Brawler.GPUResourceHandle;
import Brawler.ResourceAccessMode;
import Brawler.ResourceTransitionRequestManager;
import Brawler.ResourceTransitionToken;

export namespace Brawler
{
	class RenderJobBundle;
	class I_RenderContext;
}

export namespace Brawler
{
	class I_RenderJob
	{
	private:
		friend class RenderJobBundle;
		friend class I_RenderContext;

	protected:
		I_RenderJob();

	public:
		virtual ~I_RenderJob() = default;

		I_RenderJob(const I_RenderJob& rhs) = default;
		I_RenderJob& operator=(const I_RenderJob& rhs) = default;

		I_RenderJob(I_RenderJob&& rhs) noexcept = default;
		I_RenderJob& operator=(I_RenderJob&& rhs) noexcept = default;

	public:
		/// <summary>
		/// Adds a new I_GPUResource dependency for this I_RenderJob, allowing it to be accessed via an
		/// appropriate render context. If a dependency for this resource already existed for this job,
		/// then the specified resource states replace those specified by the previous call to this
		/// function.
		/// </summary>
		/// <param name="resource">
		/// - The resource which this I_RenderJob depends on.
		/// </param>
		/// <param name="resourceStates">
		/// - The resource states which resource must be in before this I_RenderJob can be executed.
		/// </param>
		/// <returns>
		/// The function returns a GPUResourceHandle which can be used by this I_RenderJob instance (and
		/// only this instance) to access the specified resource.
		/// </returns>
		GPUResourceHandle AddResourceDependency(I_GPUResource& resource, const Brawler::ResourceAccessMode accessMode);

		ResourceTransitionToken CreateResourceTransitionToken(GPUResourceHandle& hResource, const D3D12_RESOURCE_STATES desiredState);

		void InitializeResourceTransitionManager();

	private:
#ifdef _DEBUG
		std::unordered_map<I_GPUResource*, ResourceAccessMode> mResourceDependencyMap;
#endif // _DEBUG

		ResourceTransitionRequestManager mTransitionManager;
	};
}
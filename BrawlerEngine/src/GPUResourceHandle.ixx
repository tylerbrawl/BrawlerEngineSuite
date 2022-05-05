module;

export module Brawler.GPUResourceHandle;
import Brawler.ResourceAccessMode;

export namespace Brawler
{
	class I_RenderJob;
	class I_GPUResource;
	class I_RenderContext;
	class ResourceTransitionRequestManager;
	class ResourceDescriptorTable;
}

export namespace Brawler
{
	class GPUResourceHandle
	{
	private:
		friend class I_RenderJob;
		friend class I_RenderContext;
		friend class ResourceTransitionRequestManager;
		friend class ResourceDescriptorTable;

	private:
		GPUResourceHandle(I_RenderJob& owningJob, I_GPUResource& managedResource, const ResourceAccessMode accessMode);

	public:
		GPUResourceHandle(const GPUResourceHandle& rhs) = default;
		GPUResourceHandle& operator=(const GPUResourceHandle& rhs) = default;

		GPUResourceHandle(GPUResourceHandle&& rhs) noexcept = default;
		GPUResourceHandle& operator=(GPUResourceHandle&& rhs) noexcept = default;

	private:
		I_GPUResource& operator*();
		const I_GPUResource& operator*() const;

		I_GPUResource* operator->();
		const I_GPUResource* operator->() const;

#ifdef _DEBUG
		I_RenderJob* GetOwningJob();
		const I_RenderJob* GetOwningJob() const;

		ResourceAccessMode GetAccessMode() const;
#endif // _DEBUG

	private:
#ifdef _DEBUG
		// This is a pointer to the I_RenderJob which is assigned a context which
		// can use this GPUResourceHandle. In Debug builds, this is used to ensure
		// that render jobs only use the handles which they are granted access to.
		I_RenderJob* mOwningJob;

		ResourceAccessMode mAccessMode;
#endif

		// This is a pointer to the resource which this GPUResourceHandle refers
		// to.
		I_GPUResource* mResource;
	};
}
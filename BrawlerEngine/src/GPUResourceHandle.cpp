module;

module Brawler.GPUResourceHandle;

namespace Brawler
{
	GPUResourceHandle::GPUResourceHandle(I_RenderJob& owningJob, I_GPUResource& managedResource, ResourceAccessMode accessMode) :
#ifdef _DEBUG
		mOwningJob(&owningJob),
		mAccessMode(accessMode),
#endif // _DEBUG
		mResource(&managedResource)
	{}

	I_GPUResource& GPUResourceHandle::operator*()
	{
		return *mResource;
	}

	const I_GPUResource& GPUResourceHandle::operator*() const
	{
		return *mResource;
	}

	I_GPUResource* GPUResourceHandle::operator->()
	{
		return mResource;
	}

	const I_GPUResource* GPUResourceHandle::operator->() const
	{
		return mResource;
	}

#ifdef _DEBUG
	I_RenderJob* GPUResourceHandle::GetOwningJob()
	{
		return mOwningJob;
	}

	const I_RenderJob* GPUResourceHandle::GetOwningJob() const
	{
		return mOwningJob;
	}

	ResourceAccessMode GPUResourceHandle::GetAccessMode() const
	{
		return mAccessMode;
	}
#endif // _DEBUG
}
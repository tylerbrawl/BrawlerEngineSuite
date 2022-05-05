module;
#include "DxDef.h"

export module Brawler.ResourceTransitionToken;

export namespace Brawler
{
	class I_GPUResource;
	class ResourceTransitionRequestManager;
	class I_RenderContext;
}

export namespace Brawler
{
	class ResourceTransitionToken
	{
	private:
		friend class ResourceTransitionRequestManager;
		friend class I_RenderContext;

	private:
		ResourceTransitionToken(I_GPUResource& resource, const std::uint64_t requestID);

	public:
		~ResourceTransitionToken() = default;

		ResourceTransitionToken(const ResourceTransitionToken& rhs) = default;
		ResourceTransitionToken& operator=(const ResourceTransitionToken& rhs) = default;

		ResourceTransitionToken(ResourceTransitionToken&& rhs) noexcept;
		ResourceTransitionToken& operator=(ResourceTransitionToken&& rhs) noexcept;

	private:
#ifdef _DEBUG
		std::uint64_t mRequestID;
#endif // _DEBUG

		I_GPUResource* mResource;
	};
}
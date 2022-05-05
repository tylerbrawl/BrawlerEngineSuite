module;
#include <cstdint>

module Brawler.ResourceTransitionToken;

namespace Brawler
{
	ResourceTransitionToken::ResourceTransitionToken(I_GPUResource& resource, const std::uint64_t requestID) :
#ifdef _DEBUG
		mRequestID(requestID),
#endif // _DEBUG
		mResource(&resource)
	{}

	ResourceTransitionToken::ResourceTransitionToken(ResourceTransitionToken&& rhs) noexcept :
#ifdef _DEBUG
		mRequestID(rhs.mRequestID),
#endif // _DEBUG
		mResource(rhs.mResource)
	{
#ifdef _DEBUG
		rhs.mRequestID = 0;
#endif // _DEBUG

		rhs.mResource = nullptr;
	}

	ResourceTransitionToken& ResourceTransitionToken::operator=(ResourceTransitionToken&& rhs) noexcept
	{
#ifdef _DEBUG
		mRequestID = rhs.mRequestID;
		rhs.mRequestID = 0;
#endif // _DEBUG

		mResource = rhs.mResource;
		rhs.mResource = nullptr;

		return *this;
	}
}
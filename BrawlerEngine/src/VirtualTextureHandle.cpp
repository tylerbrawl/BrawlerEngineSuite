module;
#include <cassert>

module Brawler.VirtualTextureHandle;
import Brawler.VirtualTextureDatabase;

namespace Brawler
{
	VirtualTextureHandle::VirtualTextureHandle(VirtualTexture& virtualTexture) :
		mVirtualTexturePtr(&virtualTexture)
	{}

	VirtualTextureHandle::~VirtualTextureHandle()
	{
		DeleteVirtualTextureFromDatabase();
	}

	VirtualTextureHandle::VirtualTextureHandle(VirtualTextureHandle&& rhs) noexcept :
		mVirtualTexturePtr(rhs.mVirtualTexturePtr)
	{
		rhs.mVirtualTexturePtr = nullptr;
	}

	VirtualTextureHandle& VirtualTextureHandle::operator=(VirtualTextureHandle&& rhs) noexcept
	{
		DeleteVirtualTextureFromDatabase();

		mVirtualTexturePtr = rhs.mVirtualTexturePtr;
		rhs.mVirtualTexturePtr = nullptr;

		return *this;
	}

	const VirtualTexture& VirtualTextureHandle::operator*() const
	{
		assert(mVirtualTexturePtr != nullptr);
		return *mVirtualTexturePtr;
	}

	const VirtualTexture* VirtualTextureHandle::operator->() const
	{
		assert(mVirtualTexturePtr != nullptr);
		return mVirtualTexturePtr;
	}

	void VirtualTextureHandle::DeleteVirtualTextureFromDatabase()
	{
		if (mVirtualTexturePtr != nullptr) [[likely]]
		{
			VirtualTextureDatabase::GetInstance().DeleteVirtualTexture(*this);

			// The VirtualTextureDatabase should have set mVirtualTexturePtr to nullptr, if 
			// VirtualTextureDatabase::DeleteVirtualTexture() was called.
			assert(mVirtualTexturePtr == nullptr);
		}
	}
}
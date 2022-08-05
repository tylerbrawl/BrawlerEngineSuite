module;

export module Brawler.VirtualTextureHandle;
import Brawler.VirtualTexture;

namespace Brawler
{
	class VirtualTextureDatabase;
}

export namespace Brawler
{
	class VirtualTextureHandle
	{
	private:
		friend class VirtualTextureDatabase;

	public:
		VirtualTextureHandle() = default;

	private:
		explicit VirtualTextureHandle(VirtualTexture& virtualTexture);

	public:
		~VirtualTextureHandle();

		VirtualTextureHandle(const VirtualTextureHandle& rhs) = delete;
		VirtualTextureHandle& operator=(const VirtualTextureHandle& rhs) = delete;

		VirtualTextureHandle(VirtualTextureHandle&& rhs) noexcept;
		VirtualTextureHandle& operator=(VirtualTextureHandle&& rhs) noexcept;

		const VirtualTexture& operator*() const;
		const VirtualTexture* operator->() const;

	private:
		void DeleteVirtualTextureFromDatabase();

	private:
		VirtualTexture* mVirtualTexturePtr;
	};
}
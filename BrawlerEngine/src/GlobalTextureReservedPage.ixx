module;
#include <type_traits>

export module Brawler.GlobalTextureReservedPage;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureLogicalPage;

export namespace Brawler
{
	enum class VirtualTexturePageType
	{
		/// <summary>
		/// A combined page contains all of the mip levels of a virtual texture whose
		/// dimensions are smaller than that of a page. These mip levels are packed into
		/// a single page, and an algorithm was developed to get the coordinates of each
		/// mip level's origin in constant time.
		/// 
		/// Every virtual texture must at least have its combined page present in a global
		/// texture. Otherwise, the system will assert in Debug builds. Additional global
		/// textures can be added to a GlobalTextureCollection for a given format.
		/// </summary>
		COMBINED_PAGE,

		OTHER
	};
}

export namespace Brawler
{
	class GlobalTextureReservedPage
	{
	public:
		GlobalTextureReservedPage() = default;

		GlobalTextureReservedPage(const GlobalTextureReservedPage& rhs) = default;
		GlobalTextureReservedPage& operator=(const GlobalTextureReservedPage& rhs) = default;

		GlobalTextureReservedPage(GlobalTextureReservedPage&& rhs) noexcept = default;
		GlobalTextureReservedPage& operator=(GlobalTextureReservedPage&& rhs) noexcept = default;

		bool HasAllocation() const;

		template <typename T>
			requires std::is_same_v<std::decay_t<T>, VirtualTextureLogicalPage>
		void SetVirtualTexture(VirtualTexture& virtualTexture, T&& logicalPage);

		void ClearVirtualTexture();

		VirtualTexture& GetVirtualTexture();
		const VirtualTexture& GetVirtualTexture() const;

		const VirtualTextureLogicalPage& GetAllocatedLogicalPage() const;

		VirtualTexturePageType GetAllocatedPageType() const;

	private:
		VirtualTexture* mVirtualTexturePtr;
		VirtualTextureLogicalPage mLogicalPage;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
		requires std::is_same_v<std::decay_t<T>, VirtualTextureLogicalPage>
	void GlobalTextureReservedPage::SetVirtualTexture(VirtualTexture& virtualTexture, T&& logicalPage)
	{
		mVirtualTexturePtr = &virtualTexture;
		mLogicalPage = std::forward<T>(logicalPage);
	}
}
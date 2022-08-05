module;
#include <optional>
#include <DxDef.h>

export module Brawler.GlobalTexturePageSlot;
import Brawler.VirtualTextureLogicalPage;

export namespace Brawler
{
	class GlobalTexturePageSlot
	{
	public:
		GlobalTexturePageSlot() = default;

		GlobalTexturePageSlot(const GlobalTexturePageSlot& rhs) = delete;
		GlobalTexturePageSlot& operator=(const GlobalTexturePageSlot& rhs) = delete;

		GlobalTexturePageSlot(GlobalTexturePageSlot&& rhs) noexcept = delete;
		GlobalTexturePageSlot& operator=(GlobalTexturePageSlot&& rhs) noexcept = delete;

		void AddVirtualTexturePage(const VirtualTextureLogicalPage& logicalPage);
		VirtualTextureLogicalPage RemoveVirtualTexturePage();

		bool HasVirtualTexturePage() const;
		const VirtualTextureLogicalPage& GetVirtualTexturePage() const;

	private:
		std::optional<VirtualTextureLogicalPage> mStoredLogicalPage;
	};
}
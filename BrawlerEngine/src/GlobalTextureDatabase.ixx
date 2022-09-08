module;
#include <cassert>
#include <span>
#include <DxDef.h>

export module Brawler.GlobalTextureDatabase;
import Brawler.GlobalTextureCollection;
import Brawler.VirtualTextureLogicalPage;
import Brawler.GlobalTexturePageIdentifier;
import Brawler.GlobalTextureUpdateContext;

export namespace Brawler
{
	class GlobalTextureDatabase final
	{
	private:
		GlobalTextureDatabase() = default;

	public:
		~GlobalTextureDatabase() = default;

		GlobalTextureDatabase(const GlobalTextureDatabase& rhs) = delete;
		GlobalTextureDatabase& operator=(const GlobalTextureDatabase& rhs) = delete;

		GlobalTextureDatabase(GlobalTextureDatabase&& rhs) noexcept = delete;
		GlobalTextureDatabase& operator=(GlobalTextureDatabase&& rhs) noexcept = delete;

		static GlobalTextureDatabase& GetInstance();

		void AddVirtualTexturePage(GlobalTextureUpdateContext& context, const VirtualTextureLogicalPage& logicalPage);
		void ClearGlobalTexturePages(const std::span<const GlobalTexturePageIdentifier> pageIdentifierSpan);

		void NotifyGlobalTextureForUseInCurrentFrame(const GlobalTexturePageIdentifier pageIdentifier);

		void TryDefragmentGlobalTexturesWeak(GlobalTextureUpdateContext& context);
		void TryDefragmentGlobalTexturesStrong(GlobalTextureUpdateContext& context);

	private:
		template <typename Callback>
		void ExecuteCallbackForEachCollection(const Callback& callback);

		template <typename Callback>
		void ExecuteCallbackForFormat(const DXGI_FORMAT format, const Callback& callback);

		template <typename Callback>
		void ExecuteCallbackForGlobalTextureID(const std::uint8_t desiredID, const Callback& callback);

	private:
		// Add a single GlobalTextureCollection instance for each expected DXGI_FORMAT
		// for which GlobalTextures should be made.

		GlobalTextureCollection<DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB> mBC7UnormSRGBCollection;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename Callback>
	void GlobalTextureDatabase::ExecuteCallbackForEachCollection(const Callback& callback)
	{
		callback(mBC7UnormSRGBCollection);

		// Add additional entries as extra GlobalTextureCollection instances are added.
	}

	template <typename Callback>
	void GlobalTextureDatabase::ExecuteCallbackForFormat(const DXGI_FORMAT format, const Callback& callback)
	{
		switch (format)
		{
		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB:
		{
			callback(mBC7UnormSRGBCollection);
			break;
		}

		default:
		{
			assert(false && "ERROR: An unrecognized DXGI_FORMAT was specified in a call to GlobalTextureDatabase::ExecuteCallbackForFormat()! (Did you create a GlobalTextureCollection instance for this format and add it to this function?)");
			std::unreachable();
		}
		}
	}

	template <typename Callback>
	void GlobalTextureDatabase::ExecuteCallbackForGlobalTextureID(const std::uint8_t desiredID, const Callback& callback)
	{
		if (mBC7UnormSRGBCollection.HasGlobalTextureID(desiredID))
			callback(mBC7UnormSRGBCollection);

		// Add additional entries as extra GlobalTextureCollection instances are added.
	}
}
module;
#include <array>
#include <memory>
#include <mutex>

export module Brawler.VirtualTextureDatabase;
import Brawler.GPUSceneLimits;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureHandle;
import Brawler.FilePathHash;
import Brawler.ThreadSafeVector;
import Brawler.Functional;

export namespace Brawler
{
	class VirtualTextureDatabase final
	{
	private:
		struct VirtualTextureStorage
		{
			std::unique_ptr<VirtualTexture> VirtualTexturePtr;
			mutable std::mutex CritSection;
		};

	private:
		VirtualTextureDatabase() = default;

	public:
		~VirtualTextureDatabase() = default;

		VirtualTextureDatabase(const VirtualTextureDatabase& rhs) = delete;
		VirtualTextureDatabase& operator=(const VirtualTextureDatabase& rhs) = delete;

		VirtualTextureDatabase(VirtualTextureDatabase&& rhs) noexcept = delete;
		VirtualTextureDatabase& operator=(VirtualTextureDatabase&& rhs) noexcept = delete;

		static VirtualTextureDatabase& GetInstance();

		VirtualTextureHandle CreateVirtualTexture(const FilePathHash bvtxFileHash);
		void DeleteVirtualTexture(VirtualTextureHandle& hVirtualTexture);

		template <Brawler::Function<void, VirtualTexture&> Callback>
		bool AccessVirtualTexture(const std::uint32_t virtualTextureID, const Callback& callback);

		template <Brawler::Function<void, const VirtualTexture&> Callback>
		bool AccessVirtualTexture(const std::uint32_t virtualTextureID, const Callback& callback) const;

	private:
		/// <summary>
		/// This is essentially a map between VirtualTexture IDs and their corresponding
		/// current VirtualTexture instance. Rather than using a std::unordered_map, however,
		/// we can just use a std::array and index based on their ID value.
		/// 
		/// We use std::unique_ptr&lt;VirtualTexture&gt; instances rather than just VirtualTexture
		/// instances because it is highly likely that only a small portion of the array
		/// will contain actual VirtualTexture data at any given time, as well as for the added
		/// benefit of pointer stability.
		/// </summary>
		std::array<VirtualTextureStorage, MAX_VIRTUAL_TEXTURE_DESCRIPTIONS> mVirtualTexturePtrArr;

		ThreadSafeVector<std::unique_ptr<VirtualTexture>> mPendingDeletionArr;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <Brawler::Function<void, VirtualTexture&> Callback>
	bool VirtualTextureDatabase::AccessVirtualTexture(const std::uint32_t virtualTextureID, const Callback& callback)
	{
		std::scoped_lock<std::mutex> lock{ mVirtualTexturePtrArr[virtualTextureID].CritSection };

		{
			const std::unique_ptr<VirtualTexture>& virtualTexturePtr{ mVirtualTexturePtrArr[virtualTextureID].VirtualTexturePtr };

			if (virtualTexturePtr == nullptr) [[unlikely]]
				return false;

			callback(*virtualTexturePtr);
		}
		
		return true;
	}

	template <Brawler::Function<void, const VirtualTexture&> Callback>
	bool VirtualTextureDatabase::AccessVirtualTexture(const std::uint32_t virtualTextureID, const Callback& callback) const
	{
		std::scoped_lock<std::mutex> lock{ mVirtualTexturePtrArr[virtualTextureID].CritSection };

		{
			const std::unique_ptr<VirtualTexture>& virtualTexturePtr{ mVirtualTexturePtrArr[virtualTextureID].VirtualTexturePtr };

			if (virtualTexturePtr == nullptr) [[unlikely]]
				return false;

			callback(*virtualTexturePtr);
		}

		return true;
	}
}
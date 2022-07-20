module;
#include <array>
#include <memory>

export module Brawler.VirtualTextureDatabase;
import Brawler.GPUSceneLimits;
import Brawler.VirtualTexture;
import Brawler.VirtualTextureHandle;
import Brawler.FilePathHash;
import Brawler.ThreadSafeVector;

export namespace Brawler
{
	class VirtualTextureDatabase final
	{
	private:
		struct PendingVirtualTextureDeletion
		{
			std::unique_ptr<VirtualTexture> VirtualTexturePtr;
			std::uint64_t DeletionFrameNumber;
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

	private:
		/// <summary>
		/// This is essentially a map between VirtualTexture IDs and their corresponding
		/// current VirtualTexture instance. Rather than using a std::unordered_map, however,
		/// we can just use a std::array and index based on their ID value.
		/// 
		/// We use std::unique_ptr&lt;VirtualTexture&gt; instances rather than just VirtualTexture
		/// instances because it is highly likely that only a small portion of the array
		/// will contain actual VirtualTexture data at any given time.
		/// </summary>
		std::array<std::unique_ptr<VirtualTexture>, MAX_VIRTUAL_TEXTURE_DESCRIPTIONS> mVirtualTexturePtrArr;

		ThreadSafeVector<PendingVirtualTextureDeletion> mPendingDeletionArr;
	};
}
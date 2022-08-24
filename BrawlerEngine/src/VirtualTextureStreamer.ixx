module;
#include <atomic>

export module Brawler.VirtualTextureStreamer;
import Brawler.VirtualTextureLogicalPage;
import Brawler.ThreadSafeVector;

export namespace Brawler
{
	class VirtualTextureStreamer final
	{
	private:
		VirtualTextureStreamer() = default;

	public:
		~VirtualTextureStreamer() = default;

		VirtualTextureStreamer(const VirtualTextureStreamer& rhs) = delete;
		VirtualTextureStreamer& operator=(const VirtualTextureStreamer& rhs) = delete;

		VirtualTextureStreamer(VirtualTextureStreamer&& rhs) noexcept = delete;
		VirtualTextureStreamer& operator=(VirtualTextureStreamer&& rhs) noexcept = delete;

		static VirtualTextureStreamer& GetInstance();

		/// <summary>
		/// This function is called to do one of two things:
		/// 
		///   - If the logical virtual texture page specified by logicalPage is already present in
		///     a GlobalTexture, then the last-used frame number for this page data is updated.
		/// 
		///   - If the logical virtual texture page specified by logicalPage is *NOT* already present
		///     in a GlobalTexture, then a request is made so that during the next virtual texture
		///     streaming pass, the appropriate data will be uploaded to the GPU.
		/// </summary>
		/// <param name="logicalPage">
		/// - The VirtualTextureLogicalPage instance describing the logical virtual texture page
		///   which will either be streamed in or have its last-used frame number be updated.
		/// </param>
		void UpdateLogicalPage(const VirtualTextureLogicalPage logicalPage);

		void ExecuteStreamingPassAsync();

	private:
		void BeginStreamingPass();

	private:
		ThreadSafeVector<VirtualTextureLogicalPage> mPendingRequestArr;
		std::atomic<bool> mIsEarlierPassRunning;
	};
}
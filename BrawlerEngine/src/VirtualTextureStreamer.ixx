module;
#include <atomic>
#include <queue>
#include <memory>
#include <span>

export module Brawler.VirtualTextureStreamer;
import Brawler.VirtualTextureLogicalPage;
import Brawler.ThreadSafeVector;
import Brawler.GlobalTextureUpdateContext;

export namespace Brawler
{
	class VirtualTextureStreamer final
	{
	public:
		enum class GlobalTextureDefragmentationType
		{
			/// <summary>
			/// This is used to indicate that no defragmentation needs to occur. Passing this
			/// to VirtualTextureStreamer::RequestGlobalTextureDefragmentation() will assert in
			/// Debug builds, since doing this is a waste of CPU time.
			/// </summary>
			NONE,

			/// <summary>
			/// During weak defragmentation of GlobalTexture instances, a GlobalTexture A has
			/// its page data moved to the GlobalTextures in SetA = {B, C, D, ...} iff there
			/// are enough *EMPTY* slots in all of SetA to contain all of the page data found
			/// in A.
			/// 
			/// This implies that texture data is never lost during weak defragmentation. This
			/// is in contrast to strong defragmentation, in which high-LOD page data might be
			/// overwritten.
			/// </summary>
			WEAK,

			/// <summary>
			/// During strong defragmentation of GlobalTexture instances, a GlobalTexture A has
			/// its page data moved to the GlobalTextures in SetA = {B, C, D, ...} iff there are
			/// enough slots in SetA *NOT BEING USED FOR COMBINED PAGE DATA* to hold *AT LEAST*
			/// all of the combined page data found in A.
			/// 
			/// High-LOD page data found in SetA might be overwritten or lost as a result of strong 
			/// defragmentation. For instance, a combined page in A might overwrite page data for a 
			/// low logical mip level in a GlobalTexture found in SetA. This might be acceptable if,
			/// e.g., texture detail is being dropped for whatever reason, but it could also be
			/// undesirable.
			/// 
			/// To perform defragmentation without allowing for the loss of high-LOD page data,
			/// use weak defragmentation, instead. The chances of it being able to free GPU memory
			/// are lower, but it will never result in lost data.
			/// </summary>
			STRONG
		};

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

		void RequestGlobalTextureDefragmentation(const GlobalTextureDefragmentationType defragType);

		void ExecuteStreamingPassAsync();

	private:
		void BeginStreamingPass();

		static void HandleExtractedRequests(GlobalTextureUpdateContext& context, const std::span<const VirtualTextureLogicalPage> requestedPageSpan);
		void HandleDefragmentationRequest(GlobalTextureUpdateContext& context);

		void CheckForPendingUpdateContextSubmissions();

	private:
		ThreadSafeVector<VirtualTextureLogicalPage> mPendingRequestArr;
		std::atomic<bool> mIsEarlierPassRunning;
		std::atomic<GlobalTextureDefragmentationType> mRequestedDefragType;
		std::queue<std::unique_ptr<GlobalTextureUpdateContext>> mPendingUpdateContextQueue;
	};
}
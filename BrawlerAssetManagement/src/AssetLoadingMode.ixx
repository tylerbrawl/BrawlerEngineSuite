module;
#include <cstdint>

export module Brawler.AssetManagement.AssetLoadingMode;

export namespace Brawler
{
	namespace AssetManagement
	{
		/// <summary>
		/// The AssetLoadingMode enumeration specifies how many threads should be handling asset I/O
		/// requests at any given time. Rather than using a fixed number of threads, the actual amount
		/// used is calculated at run-time bassed on the number of logical cores in the user's system
		/// (i.e., based on std::thread::hardware_concurrency()).
		/// 
		/// At runtime, the AssetLoadingMode of the AssetManager can be changed by calling
		/// AssetManager::SetAssetLoadingMode(). The actual semantic meaning of this value differs
		/// based on the asset I/O request handler chosen at run-time. For instance, since the
		/// DirectStorage API handles its requests asynchronously on separate threads, changing this
		/// value when using DirectStorage only affects the number of threads dedicated to decompression.
		/// (The DirectStorage API does allow you to change the number of threads active, but this can
		/// only be done before creating the IDStorageFactory.)
		/// </summary>
		enum class AssetLoadingMode
		{
			/// <summary>
			/// At most one thread will be dedicated to handling asset requests. This is probably
			/// the best option in most cases during runtime.
			/// </summary>
			MINIMAL_OVERHEAD,

			/// <summary>
			/// Either (0.25f * std::thread::hardware_concurrency()) threads or a single thread is dedicated 
			/// to handling asset requests, whichever is larger. This can be a good choice when a gameplay
			/// scenario requires data to be streamed in quickly.
			/// </summary>
			OPTIMIZE_FOR_RUNTIME,

			/// <summary>
			/// Either (0.60f * std::thread::hardware_concurrency()) threads or a single thread is dedicated 
			/// to handling asset requests, whichever is larger. This is probably the best choice
			/// during situations such as loading screens, where assets should be loaded quickly but we
			/// still want some threads available for, e.g., drawing animated loading screens without
			/// major stuttering.
			/// </summary>
			OPTIMIZE_FOR_LOADING,

			/// <summary>
			/// std::thread::hardware_concurrency() threads are dedicated to handling asset requests.
			/// 
			/// This option should only be used when absolutely necessary, for obvious reasons.
			/// </summary>
			LOAD_OR_DIE_TRYING,

			COUNT_OR_ERROR
		};

		std::uint32_t GetSuggestedThreadCountForAssetIORequests(const AssetLoadingMode loadingMode);
	}
}
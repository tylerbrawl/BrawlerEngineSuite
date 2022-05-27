module;

export module Brawler.AssetManagement.AssetLoadingMode;

export namespace Brawler
{
	namespace AssetManagement
	{
		enum class AssetLoadingMode
		{
			/// <summary>
			/// At most one thread will be dedicated to handling asset requests. This is probably
			/// the best option in most cases during runtime.
			/// </summary>
			MINIMAL_OVERHEAD,

			/// <summary>
			/// Either (0.25f * std::hardware_concurrency()) or (std::hardware_concurrency() - 1)
			/// threads are dedicated to handling asset requests, whichever is smaller. This can
			/// be a good choice when a gameplay scenario requires data to be streamed in quickly.
			/// </summary>
			OPTIMIZE_FOR_RUNTIME,

			/// <summary>
			/// Either (0.60f * std::hardware_concurrency()) or (std::hardware_concurrency() - 1)
			/// threads are dedicated to handling asset requests, whichever is smaller. This is
			/// probably the best choice during situations such as loading screens, where assets
			/// should be loaded quickly but we still want some threads available for, e.g.,
			/// drawing animated loading screens without major stuttering.
			/// </summary>
			OPTIMIZE_FOR_LOADING,

			/// <summary>
			/// This mode functions differently from the other ones. Rather than having X threads
			/// handle asset requests, when the time comes to ensure that asset requests are being
			/// handled, the calling thread is blocked until ALL requests are handled.
			/// 
			/// This option should only be used when absolutely necessary, for obvious reasons.
			/// </summary>
			LOAD_OR_DIE_TRYING,

			COUNT_OR_ERROR
		};
	}
}
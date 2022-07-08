module;
#include <atomic>
#include <vector>
#include <mutex>
#include <array>
#include <functional>
#include "DxDef.h"

export module Brawler.D3D12.PresentationManager;
import Util.Engine;
import Brawler.D3D12.GPUCommandQueue;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.CompositeEnum;

export namespace Brawler
{
	namespace D3D12
	{
		class PresentationManager
		{
		public:
			struct PresentationInfo
			{
				/// <summary>
				/// This is the frame number for which presentation is to be completed.
				/// </summary>
				std::uint64_t FrameNumber;

				/// <summary>
				/// This is a combination of all of the GPUCommandQueues from the GPUCommandManager
				/// which the presentation queue must wait on before any presentation commands can be
				/// executed. In C++ terms, the execution of the presentation commands on the GPU
				/// timeline "synchronizes with" the signal of the GPUFence associated with the specified
				/// command queues.
				/// 
				/// For the best performance, the minimum number of queues should be specified in order
				/// to ensure correctness. Obviously, the DIRECT queue is a requirement, but the (asynchronous)
				/// COMPUTE queue might also need synchronization.
				/// </summary>
				Brawler::CompositeEnum<GPUCommandQueueType> QueuesToSynchronizeWith;
			};

		public:
			PresentationManager() = default;

			PresentationManager(const PresentationManager& rhs) = delete;
			PresentationManager& operator=(const PresentationManager& rhs) = delete;

			PresentationManager(PresentationManager&& rhs) noexcept = default;
			PresentationManager& operator=(PresentationManager&& rhs) noexcept = default;

			void Initialize();

			void EnablePresentationForCurrentFrame();

			/// <summary>
			/// If presentation is enabled for the current frame, then this function will
			/// have the presentation queue wait on all of the fences signalled by the GPUCommandQueue
			/// instances in the GPUCommandManager during the last submission event. Then, it will call each 
			/// callback registered with the PresentationManager instance through 
			/// PresentationManager::RegisterPresentationCallback().
			/// 
			/// This function is meant to be called by the thread which is submitting commands to the
			/// GPU for the current frame after it has finished submitting all of its commands for said
			/// frame. The return value of the function indicates whether any presentation calls were
			/// made as a result of calling the function.
			/// </summary>
			/// <param name="presentationInfo">
			/// - A const PresentationManager::PresentationInfo& which describes both the frame number
			///   to present for and the command queues to synchronize with. These values must be explicitly
			///   passed to this function, rather than inferred via calls to functions like
			///   Util::Engine::GetCurrentFrameNumber(), in order to prevent potential race conditions.
			/// </param>
			/// <returns>
			/// The function returns true if every registered presentation callback was called as a 
			/// result of calling the function and false otherwise.
			/// </returns>
			bool HandleFramePresentation(const PresentationInfo& presentationInfo);

			GPUCommandQueue<GPUCommandQueueType::DIRECT>& GetPresentationCommandQueue();
			const GPUCommandQueue<GPUCommandQueueType::DIRECT>& GetPresentationCommandQueue() const;

			/// <summary>
			/// Registers the specified callback function with this PresentationManager instance. The function
			/// will be called immediately after the commands for a frame have been submitted to the GPU, but
			/// only if DirectContext::Present() was called in at least one relevant RenderPass.
			/// 
			/// The specified callback should return an HRESULT to indicate whether or not the presentation
			/// succeeded. Each callback registered with the PresentationManager *MAY* be executed concurrently,
			/// but this is not a guarantee. The method for deciding this is left unspecified.
			/// 
			/// Technically, any function could be provided here and the code would be executed and work fine.
			/// However, the provided functions *MUST* be thread safe! The behavior is *undefined* if the callback
			/// does anything which violates this requirement.
			/// 
			/// The idea is that IDXGISwapChain::Present1() gets called by the end of the callback's execution,
			/// but again, the callback could theoretically execute any thread-safe function and work just fine.
			/// One might wonder why the user cannot just specify IDXGISwapChain instances to be presented
			/// automatically by the PresentationManager. The real reason as to why it was done this way is because
			/// the actual classes meant to represent swap chains are likely to differ based on the application.
			/// For instance, a swap chain for a VR application might have very different presentation requirements
			/// from those which create a swap chain for a standard Win32 window.
			/// 
			/// As mentioned, the HRESULT returned by the callback should indicate the function's success. All callbacks
			/// are executed, even if one of them fails. However, if any callback reports a failure, then an exception
			/// is thrown by the thread which submits GPU commands; this will ultimately lead to program termination.
			/// If this is undesirable, then simply return S_OK on failure and resolve the issue at a later time.
			/// </summary>
			/// <param name="callback">
			/// - The callback function which is to be registered with the PresentationManager instance. The function
			///   *MUST* be thread safe!
			/// </param>
			void RegisterPresentationCallback(std::move_only_function<HRESULT()>&& callback);

			void ClearPresentationCallbacks();

		private:
			void SubmitPresentCommands(const PresentationInfo& presentationInfo);

		private:
			GPUCommandQueue<GPUCommandQueueType::DIRECT> mPresentationQueue;
			std::array<std::atomic<bool>, Util::Engine::MAX_FRAMES_IN_FLIGHT> mPresentEnabledArr;
			std::vector<std::move_only_function<HRESULT()>> mPresentationCallbackArr;
			mutable std::mutex mPresentationCallbackArrCritSection;
		};
	}
}
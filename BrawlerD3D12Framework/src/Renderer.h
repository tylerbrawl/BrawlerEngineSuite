#pragma once
#include <cstdint>

import Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.GPUDevice;
import Brawler.D3D12.PersistentGPUResourceManager;
import Brawler.D3D12.RootSignatureDatabase;  // <- Safe?
import Brawler.D3D12.PSODatabase;  // <- Safe?
import Brawler.D3D12.FrameGraphManager;
import Brawler.D3D12.PresentationManager;  // <- Safe?
import Brawler.D3D12.I_RenderModule;  // <- Safe?
import Brawler.Functional;  // <- Safe?

namespace Brawler
{
	namespace D3D12
	{
		class Renderer
		{
		public:
			Renderer() = default;

			Renderer(const Renderer& rhs) = delete;
			Renderer& operator=(const Renderer& rhs) = delete;

			Renderer(Renderer&& rhs) noexcept = default;
			Renderer& operator=(Renderer&& rhs) noexcept = default;

			void Initialize();

			GPUCommandManager& GetGPUCommandManager();
			const GPUCommandManager& GetGPUCommandManager() const;

			GPUDevice& GetGPUDevice();
			const GPUDevice& GetGPUDevice() const;

			PersistentGPUResourceManager& GetPersistentGPUResourceManager();
			const PersistentGPUResourceManager& GetPersistentGPUResourceManager() const;

			PresentationManager& GetPresentationManager();
			const PresentationManager& GetPresentationManager() const;

			FrameGraphManager& GetFrameGraphManager();
			const FrameGraphManager& GetFrameGraphManager() const;

			template <typename T, typename... Args>
				requires std::derived_from<T, I_RenderModule>
			void AddRenderModule(Args&&... args);

			template <typename T>
				requires std::derived_from<T, I_RenderModule>
			T& GetRenderModule();

			template <typename T>
				requires std::derived_from<T, I_RenderModule>
			const T& GetRenderModule() const;

			/// <summary>
			/// Adds the callback function specified by callback as a persistent FrameGraph completion
			/// callback function. Each FrameGraph completion callback function is executed after the
			/// FrameGraph has waited for the GPU to execute the commands submitted to it
			/// Util::Engine::MAX_FRAMES_IN_FLIGHT frames prior and after all of the resources used
			/// in said commands (e.g., transient resources, per-frame descriptor heap allocations, etc.)
			/// have been cleaned up, but before FrameGraph compilation begins for the current frame.
			/// 
			/// By "persistent," we mean that the callback function remains registered with the
			/// FrameGraph for the remainder of the program's execution. This is in contrast to transient
			/// FrameGraph completion callback functions, which are deleted after they are executed.
			/// 
			/// *NOTE*: ALL FrameGraph completion callback functions are executed concurrently as CPU
			/// jobs. If you need to ensure that a series of tasks are executed in a specific order, then
			/// move these tasks into a single callback function.
			/// </summary>
			/// <typeparam name="Callback">
			/// - The type of callback must have the function signature void().
			/// </typeparam>
			/// <param name="callback">
			/// - The callback function which is to be added as a persistent FrameGraph completion callback
			///   function.
			/// </param>
			template <Brawler::Function<void> Callback>
			void AddPersistentFrameGraphCompletionCallback(Callback&& callback);

			/// <summary>
			/// Adds the callback function specified by callback as a transient FrameGraph completion
			/// callback function. Each FrameGraph completion callback function is executed after the
			/// FrameGraph has waited for the GPU to execute the commands submitted to it
			/// Util::Engine::MAX_FRAMES_IN_FLIGHT frames prior and after all of the resources used
			/// in said commands (e.g., transient resources, per-frame descriptor heap allocations, etc.)
			/// have been cleaned up, but before FrameGraph compilation begins for the current frame.
			/// 
			/// By "transient," we mean that the callback function is removed from the FrameGraph after
			/// it has been executed once. This is in contrast to persistent FrameGraph completion callback
			/// functions, which remain registered with the FrameGraph for the remainder of the program's
			/// execution.
			/// 
			/// By their nature, transient FrameGraph completion callback functions are great when you need
			/// to perform some task only when you know that the GPU has finished executing prior commands
			/// for a given frame. This can be used to, e.g., read from a READBACK buffer as soon as it is
			/// guaranteed that its contents have been initialized. However, similar behavior can also be
			/// achieved by keeping track of the current frame number. This method tends to be less intuitive
			/// in certain scenarios, but it can be more performant than registering a transient FrameGraph
			/// completion callback function.
			/// 
			/// *NOTE*: ALL FrameGraph completion callback functions are executed concurrently as CPU
			/// jobs. If you need to ensure that a series of tasks are executed in a specific order, then
			/// move these tasks into a single callback function.
			/// </summary>
			/// <typeparam name="Callback">
			/// - The type of callback must have the function signature void().
			/// </typeparam>
			/// <param name="callback">
			/// - The callback function which is to be added as a transient FrameGraph completion callback
			///   function.
			/// </param>
			template <Brawler::Function<void> Callback>
			void AddTransientFrameGraphCompletionCallback(Callback&& callback);

			/// <summary>
			/// Generates and submits the FrameGraph for the current frame. All enabled
			/// I_RenderModule instances which are owned by the Renderer's FrameGraphManager
			/// instance are then instructed to create RenderPass instances for the frame.
			/// 
			/// Although there is no way to remove I_RenderModule instances once added,
			/// a derived I_RenderModule instance will be "skipped" during FrameGraph generation
			/// if its I_RenderModule::IsRenderModuleEnabled() function returns false. This
			/// is helpful if, e.g., a set of RenderPasses should only be created if a certain
			/// configuration option is set.
			/// </summary>
			void ProcessFrame();

			/// <summary>
			/// Executes some tasks which must be completed after FrameGraph generation and
			/// submission and then increments the current frame number. 
			/// 
			/// Any function which needs the current frame number to be accurate should be 
			/// called *before* this function is called; otherwise, you might get race conditions 
			/// and other weird bugs and issues.
			/// </summary>
			void AdvanceFrame();

			std::uint64_t GetCurrentFrameNumber() const;

		private:
			GPUDevice mDevice;
			GPUCommandManager mCmdManager;
			PersistentGPUResourceManager mPersistentResourceManager;
			PresentationManager mPresentationManager;
			FrameGraphManager mFrameGraphManager;
			std::atomic<std::uint64_t> mCurrFrameNum;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename T, typename... Args>
			requires std::derived_from<T, I_RenderModule>
		void Renderer::AddRenderModule(Args&&... args)
		{
			mFrameGraphManager.AddRenderModule<T>(std::forward<Args>(args)...);
		}

		template <typename T>
			requires std::derived_from<T, I_RenderModule>
		T& Renderer::GetRenderModule()
		{
			return mFrameGraphManager.GetRenderModule<T>();
		}

		template <typename T>
			requires std::derived_from<T, I_RenderModule>
		const T& Renderer::GetRenderModule() const
		{
			return mFrameGraphManager.GetRenderModule<T>();
		}

		template <Brawler::Function<void> Callback>
		void Renderer::AddPersistentFrameGraphCompletionCallback(Callback&& callback)
		{
			mFrameGraphManager.AddPersistentFrameGraphCompletionCallback(std::forward<Callback>(callback));
		}

		template <Brawler::Function<void> Callback>
		void Renderer::AddTransientFrameGraphCompletionCallback(Callback&& callback)
		{
			mFrameGraphManager.AddTransientFrameGraphCompletionCallback(std::forward<Callback>(callback));
		}
	}
}
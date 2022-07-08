module;
#include <atomic>

export module Brawler.D3D12.Renderer;
import Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.GPUDevice;
import Brawler.D3D12.PersistentGPUResourceManager;
import Brawler.D3D12.RootSignatureDatabase;
import Brawler.D3D12.PSODatabase;
import Brawler.D3D12.FrameGraphManager;
import Brawler.D3D12.PresentationManager;
import Brawler.D3D12.I_RenderModule;

export namespace Brawler
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
	}
}
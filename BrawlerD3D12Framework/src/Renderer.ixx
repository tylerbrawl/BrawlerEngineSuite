module;
#include <atomic>

export module Brawler.D3D12.Renderer;
import Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.GPUDevice;
import Brawler.D3D12.PersistentGPUResourceManager;
import Brawler.D3D12.RootSignatureDatabase;
import Brawler.D3D12.PSODatabase;
import Brawler.D3D12.FrameGraphManager;
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

			template <typename T, typename... Args>
				requires std::derived_from<T, I_RenderModule>
			void AddRenderModule(Args&&... args);

			template <typename T>
				requires std::derived_from<T, I_RenderModule>
			T& GetRenderModule();

			template <typename T>
				requires std::derived_from<T, I_RenderModule>
			const T& GetRenderModule() const;

			void ProcessFrame();
			void AdvanceFrame();
			std::uint64_t GetCurrentFrameNumber() const;

		private:
			GPUDevice mDevice;
			GPUCommandManager mCmdManager;
			PersistentGPUResourceManager mPersistentResourceManager;
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
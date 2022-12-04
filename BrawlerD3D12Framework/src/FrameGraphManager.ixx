module;
#include <vector>
#include <memory>
#include <array>
#include <cassert>
#include <functional>
#include "DxDef.h"

export module Brawler.D3D12.FrameGraphManager;
import Brawler.D3D12.FrameGraph;
import Util.Engine;
import Brawler.D3D12.I_RenderModule;
import Brawler.D3D12.GPUCommandQueueType;

// No RTTI? No problem!
namespace Brawler
{
	namespace D3D12
	{
		static constexpr std::size_t INVALID_INDEXER_VALUE = std::numeric_limits<std::size_t>::max();
		
		template <typename T>
		struct RenderModuleIndexer
		{
			static inline std::size_t IndexerValue = INVALID_INDEXER_VALUE;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphManager
		{
		public:
			FrameGraphManager() = default;

			FrameGraphManager(const FrameGraphManager& rhs) = delete;
			FrameGraphManager& operator=(const FrameGraphManager& rhs) = delete;

			FrameGraphManager(FrameGraphManager&& rhs) noexcept = default;
			FrameGraphManager& operator=(FrameGraphManager&& rhs) noexcept = default;

			void Initialize();

			void ProcessCurrentFrame();

			void AddPersistentFrameGraphCompletionCallback(std::move_only_function<void()>&& persistentCallback);
			void AddTransientFrameGraphCompletionCallback(std::move_only_function<void()>&& transientCallback);

			template <typename T, typename... Args>
				requires std::derived_from<T, I_RenderModule>
			void AddRenderModule(Args&&... args);

			template <typename T>
				requires std::derived_from<T, I_RenderModule>
			T& GetRenderModule();

			template <typename T>
				requires std::derived_from<T, I_RenderModule>
			const T& GetRenderModule() const;

			Brawler::D3D12CommandAllocator& GetD3D12CommandAllocator(const GPUCommandQueueType queueType);

		private:
			FrameGraph& GetCurrentFrameGraph();
			const FrameGraph& GetCurrentFrameGraph() const;

		private:
			std::array<FrameGraph, Util::Engine::MAX_FRAMES_IN_FLIGHT> mFrameGraphArr;
			std::vector<std::unique_ptr<I_RenderModule>> mRenderModuleArr;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename T, typename... Args>
			requires std::derived_from<T, I_RenderModule>
		void FrameGraphManager::AddRenderModule(Args&&... args)
		{
			assert(RenderModuleIndexer<T>::IndexerValue == INVALID_INDEXER_VALUE && "ERROR: An attempt was made to add more than one instance of the same type of I_RenderModule to the FrameGraphManager!");

			// Save the current index for this type in the static structure. This works because
			// we exploit a number of factors:
			//
			//  - Only one FrameGraphManager instance exists, and this instance is owned by
			//    the Renderer.
			//
			//  - We only allow one instance of any given derived class of I_RenderModule
			//    to exist.
			RenderModuleIndexer<T>::IndexerValue = mRenderModuleArr.size();
			
			mRenderModuleArr.push_back(std::make_unique<T>(std::forward<Args>(args)...));
		}

		template <typename T>
			requires std::derived_from<T, I_RenderModule>
		T& FrameGraphManager::GetRenderModule()
		{
			assert(RenderModuleIndexer<T>::IndexerValue != INVALID_INDEXER_VALUE && "ERROR: An attempt was made to get an I_RenderModule instance of a given type before it was created!");
			return static_cast<T&>(*(mRenderModuleArr[RenderModuleIndexer<T>::IndexerValue]));
		}

		template <typename T>
			requires std::derived_from<T, I_RenderModule>
		const T& FrameGraphManager::GetRenderModule() const
		{
			assert(RenderModuleIndexer<T>::IndexerValue != INVALID_INDEXER_VALUE && "ERROR: An attempt was made to get an I_RenderModule instance of a given type before it was created!");
			return static_cast<T&>(*(mRenderModuleArr[RenderModuleIndexer<T>::IndexerValue]));
		}
	}
}
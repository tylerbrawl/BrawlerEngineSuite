module;
#include <functional>
#include <memory>

export module Brawler.D3D12.FrameGraph:FrameGraphCallbackCollection;
import Brawler.ThreadSafeVector;

export namespace Brawler 
{
	namespace D3D12
	{
		class FrameGraphCallbackCollection
		{
		private:
			using CallbackType = std::move_only_function<void()>;

		public:
			FrameGraphCallbackCollection() = default;

			FrameGraphCallbackCollection(const FrameGraphCallbackCollection& rhs) = delete;
			FrameGraphCallbackCollection& operator=(const FrameGraphCallbackCollection& rhs) = delete;

			FrameGraphCallbackCollection(FrameGraphCallbackCollection&& rhs) noexcept = delete;
			FrameGraphCallbackCollection& operator=(FrameGraphCallbackCollection&& rhs) noexcept = delete;

			void AddTransientCallback(CallbackType&& callback);

			void ExecuteFrameGraphCompletionCallbacks(const Brawler::ThreadSafeVector<std::unique_ptr<CallbackType>>& persistentCallbackArr);

		private:
			Brawler::ThreadSafeVector<CallbackType> mTransientCallbackArr;
		};
	}
}
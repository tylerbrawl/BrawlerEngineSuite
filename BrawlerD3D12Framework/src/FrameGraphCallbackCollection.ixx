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

			void AddPersistentCallback(CallbackType&& callback);
			void AddTransientCallback(CallbackType&& callback);

			void ExecuteFrameGraphCompletionCallbacks();

		private:
			/// <summary>
			/// We use multithreaded execution of callback functions, which is why we need to
			/// wrap each persistent CallbackType instance around a std::unique_ptr. Otherwise, 
			/// we would have a race condition where the following could potentially happen:
			/// 
			///   - Thread A calls FrameGraphCallbackCollection::ExecuteFrameGraphCallbacks() and
			///     adds a CPU job to a JobGroup which captures a persistent CallbackType instance
			///     by reference.
			/// 
			///   - Thread B calls FrameGraphCallbackCollection::AddPersistentCallback() to add a
			///     new persistent CallbackType instance to mPersistentCallbackArr. This causes
			///     the underlying std::vector to perform a re-size to store the new callback
			///     function, invalidating the reference to the persistent CallbackType instance
			///     made by Thread A.
			/// 
			///   - Some thread executes the CPU job created by Thread A and attempts to access the
			///     persistent CallbackType instance captured by reference. Since this memory
			///     location now refers to garbage, we just invoked undefined behavior.
			/// 
			/// Yes, this is slow (std::move_only_function definitely already has a virtual function
			/// call and might also have its own heap allocation), but it's necessary as shown above.
			/// </summary>
			Brawler::ThreadSafeVector<std::unique_ptr<CallbackType>> mPersistentCallbackArr;

			Brawler::ThreadSafeVector<CallbackType> mTransientCallbackArr;
		};
	}
}
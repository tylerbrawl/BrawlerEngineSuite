module;
#include <cstddef>
#include <atomic>
#include <array>
#include <optional>
#include <memory>

export module Brawler.ThreadSafeQueue;

// Brawler::ThreadSafeQueue is an (almost*) lock-free thread-safe data structure. It does not make 
// any heap allocations (excluding potential allocations done during initialization, but those will 
// likely be done on the stack, anyways).
//
// The queue itself has a fixed capacity.
//
// (*Okay, so there IS a bit of waiting done in ThreadSafeQueue::TryPop() for the race condition
// where a thread tries to get an element from an empty queue before it was written. It's done 
// using a compare/exchange loop, rather than a straight mutex, though.)

export namespace Brawler
{
	template <typename T, std::size_t NumElements>
	class ThreadSafeQueue
	{
	private:
		static_assert(NumElements > 0, "ERROR: An attempt was made to create a ThreadSafeQueue with no elements!");

	private:
		// This is used to prevent a race condition where the following happens:
		//
		//   - Thread A performs a successful compare/exchange for mPackedOffsets, stating
		//     that a new element is available.
		// 
		//   - Thread B performs a successful compare/exchange for mPackedOffsets, stating
		//     that this element has been claimed.
		//
		//   - Thread B accesses the array to try to claim an element. However, this value
		//     is invalid, because Thread A has not actually stored any value yet.
		//
		//   - Thread A stores the element in the array.
		using QueueElementPtr = std::atomic<T*>;

		struct QueueInfo
		{
			std::uint32_t BeginIndex;
			std::uint32_t EndIndex;
		};

		// tl;dr: Ensure that we have no more than UINT32_MAX elements in the queue.
		// 
		// Internally, we will use two arrays to represent the queue. The first array, mArr, stores atomic
		// T* values which reference values in the second array, mBackingMemory. Each mArr entry has a
		// corresponding mBackingMemory entry with the same index.
		// 
		// To make insertion and removal operations atomic, we need to make sure that both the beginning
		// and the end of the queue are updated within the same atomic operation; otherwise, we introduce
		// race conditions. This is done by packing two 32-bit unsigned integers into one atomic 64-bit
		// unsigned integer. The upper 32 bits of this atomic value represent the beginning index of the
		// queue, while the lower 32 bits represent the ending index. In doing this, we can create a queue
		// as a circular ring buffer around an array of fixed size.
		//
		// However, since we need to pack the beginning and ending indices into one 64-bit atomic integer,
		// we must be able to represent each index with 32 bits. Therefore, we can have at most UINT32_MAX
		// elements in the queue at once.
		static_assert(NumElements <= (std::numeric_limits<std::uint32_t>::max() - 1),
			"ERROR: An attempt was made to create a ThreadSafeQueue, but it was not possible to pack the indices of elements into an atomic integer! Reduce the size of the queue. (Do you REALLY need it to be that big, anyways?)");

	public:
		ThreadSafeQueue();

		ThreadSafeQueue(const ThreadSafeQueue& rhs) = delete;
		ThreadSafeQueue& operator=(const ThreadSafeQueue& rhs) = delete;

		ThreadSafeQueue(ThreadSafeQueue&& rhs) noexcept = delete;
		ThreadSafeQueue& operator=(ThreadSafeQueue&& rhs) noexcept = delete;

		/// <summary>
		/// Attempts to insert the element val into the queue. The function returns true if the insertion succeeded and
		/// false otherwise.
		/// </summary>
		/// <param name="val">
		/// - The value to be inserted to the back of the queue.
		/// </param>
		/// <returns>
		/// This function returns true if the insertion succeeded and false otherwise. Specifically, if the queue is
		/// full, then this function returns false.
		/// </returns>
		template <typename U = T>
			requires std::is_same_v<std::decay_t<U>, std::decay_t<T>>
		[[nodiscard("ERROR: ThreadSafeQueue::PushBack() can fail if the queue is full. A failed insertion is almost certainly not acceptable behavior.")]]
		bool PushBack(U&& val);

		/// <summary>
		/// Attempts to remove the element at the front of the queue. If the function succeeds, then the returned
		/// std::optional instance contains the element which was claimed from the queue. Otherwise, the returned
		/// std::optional instance has no valid value.
		/// </summary>
		/// <returns>
		/// If the function succeeds, then the returned std::optional instance contains the element which was
		/// claimed from the queue. Otherwise, the std::optional instance has no valid value. Specifically, if
		/// the queue is empty, then this function will fail.
		/// </returns>
		std::optional<T> TryPop();

		/// <summary>
		/// Use this to verify that the queue is lock-free.
		/// </summary>
		/// <returns>
		/// On any semi-decent compiler, this function will always return true. This is the case for the MSVC.
		/// </returns>
		static bool IsLockFree();

		/// <summary>
		/// Checks to see whether the queue is empty or not.
		/// </summary>
		/// <returns>
		/// This function returns true if the queue is empty at the time of calling this and false otherwise.
		/// Keep in mind that due to race conditions, it is entirely possible that an empty queue can become
		/// filled immediately after this is called, or vice versa.
		/// </returns>
		bool IsEmpty() const;

	private:
		std::uint64_t PackOffsets(const QueueInfo& queueInfo) const;
		QueueInfo UnpackOffsets(const std::uint64_t packedOffsets) const;

	private:
		// We actually have an array of size (NumElements + 1), and the reason for this is a
		// little strange. To check if the queue is empty, we see if BeginIndex == EndIndex.
		// Thus, we cannot use this convention to check if the queue is full; instead, during
		// ThreadSafeQueue::PushBack(), we check if adding an element would cause the two
		// indices to be equal; if so, then we say that the queue is full.
		//
		// This means that attempting to add this element will fail, meaning that there will
		// technically always be one empty slot in the array. To create a queue that can use
		// NumElements, then, we need our arrays to be of size (NumElements + 1).
		//
		// Why do we do it like this? It's because we cannot keep some way of differentiating
		// whether or not BeginIndex == EndIndex means that the queue is empty or that it is
		// full. Remember that all state changes for the queue must be atomic, and our state
		// variable is already filled to capacity (64 bits, or the size of a pointer) with
		// the indices. We COULD represent the state with a std::atomic<std::shared_ptr<StateType>>,
		// but then we would need to make heap allocations. Trust me: From a pure efficiency
		// point of view, this is better.

		std::array<QueueElementPtr, (NumElements + 1)> mArr;
		std::array<T, (NumElements + 1)> mBackingMemory;
		std::atomic<std::uint64_t> mPackedOffsets;
	};
}

// --------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T, std::size_t NumElements>
	ThreadSafeQueue<T, NumElements>::ThreadSafeQueue() :
		mArr(),
		mBackingMemory(),
		mPackedOffsets(0)
	{}

	template <typename T, std::size_t NumElements>
	template <typename U>
		requires std::is_same_v<std::decay_t<U>, std::decay_t<T>>
	bool ThreadSafeQueue<T, NumElements>::PushBack(U&& val)
	{
		std::uint64_t currOffsets = mPackedOffsets.load();
		std::size_t claimedIndex = 0;
		std::uint64_t newCurrOffsets = 0;

		// Try to claim the next spot in the queue. We do this by capturing the state of
		// the packed offsets, converting it to a QueueInfo, incrementing the EndPtr, and
		// doing a compare/exchange operation.
		do
		{
			QueueInfo desiredClaim{ UnpackOffsets(currOffsets) };

			// If the queue is full, then return false.
			if ((desiredClaim.EndIndex + 1) % mArr.size() == desiredClaim.BeginIndex)
				return false;

			// Otherwise, if the queue is *NOT* full, then proceed with setting the claim normally.
			claimedIndex = static_cast<std::size_t>(desiredClaim.EndIndex);
			desiredClaim.EndIndex = ((desiredClaim.EndIndex + 1) % mArr.size());

			newCurrOffsets = PackOffsets(desiredClaim);
		} while (!mPackedOffsets.compare_exchange_weak(currOffsets, newCurrOffsets));

		mBackingMemory[claimedIndex] = std::forward<U>(val);
		mArr[claimedIndex].store(&(mBackingMemory[claimedIndex]));

		return true;
	}

	template <typename T, std::size_t NumElements>
	std::optional<T> ThreadSafeQueue<T, NumElements>::TryPop()
	{
		std::uint64_t currOffsets = mPackedOffsets.load();
		QueueElementPtr* claimedPtr = nullptr;
		std::uint64_t newCurrOffsets = 0;

		do
		{
			QueueInfo queueInfo{ UnpackOffsets(currOffsets) };

			// If the queue is empty, then return nothing.
			if (queueInfo.BeginIndex == queueInfo.EndIndex)
				return std::optional<T>{};

			claimedPtr = &(mArr[queueInfo.BeginIndex]);
			queueInfo.BeginIndex = ((queueInfo.BeginIndex + 1) % mArr.size());

			newCurrOffsets = PackOffsets(queueInfo);
		} while (!mPackedOffsets.compare_exchange_weak(currOffsets, newCurrOffsets));

		// Wait for the element to be filled.
		T* claimedElement = nullptr;
		while (!claimedPtr->compare_exchange_weak(claimedElement, nullptr) || claimedElement == nullptr);

		return std::optional<T>{std::move(*claimedElement)};
	}

	template <typename T, std::size_t NumElements>
	bool ThreadSafeQueue<T, NumElements>::IsLockFree()
	{
		static std::atomic<std::uint64_t> atomicIntTest{};
		static std::atomic<T*> atomicPtrTest{};

		return atomicIntTest.is_lock_free() && atomicPtrTest.is_lock_free();
	}

	template <typename T, std::size_t NumElements>
	bool ThreadSafeQueue<T, NumElements>::IsEmpty() const
	{
		QueueInfo queueInfo{ UnpackOffsets(mPackedOffsets.load()) };
		return (queueInfo.BeginIndex == queueInfo.EndIndex);
	}

	template <typename T, std::size_t NumElements>
	std::uint64_t ThreadSafeQueue<T, NumElements>::PackOffsets(const QueueInfo& queueInfo) const
	{
		std::uint64_t packedOffsets = 0;

		// The upper 32 bits represent the BeginIndex.
		packedOffsets |= queueInfo.BeginIndex;
		packedOffsets <<= 32;

		// The lower 32 bits represent the EndIndex.
		packedOffsets |= queueInfo.EndIndex;

		return packedOffsets;
	}

	template <typename T, std::size_t NumElements>
	ThreadSafeQueue<T, NumElements>::QueueInfo ThreadSafeQueue<T, NumElements>::UnpackOffsets(const std::uint64_t packedOffsets) const
	{
		if (packedOffsets == 0)
			return QueueInfo{ 0, 0 };
		
		QueueInfo queueInfo{};
		queueInfo.BeginIndex = static_cast<std::uint32_t>(packedOffsets >> 32);
		queueInfo.EndIndex = static_cast<std::uint32_t>(packedOffsets & 0xFFFFFFFF);

		return queueInfo;
	}
}
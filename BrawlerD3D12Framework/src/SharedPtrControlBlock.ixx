module;
#include <cstdint>
#include <atomic>
#include <concepts>

export module Brawler.SharedPtr:SharedPtrControlBlock;

export namespace Brawler
{
	template <typename DataType>
	class SharedPtrControlBlock
	{
	public:
		SharedPtrControlBlock() = default;

		template <typename... Args>
			requires std::constructible_from<DataType, Args...>
		SharedPtrControlBlock(Args&&... args);

		SharedPtrControlBlock(const SharedPtrControlBlock& rhs) = default;
		SharedPtrControlBlock& operator=(const SharedPtrControlBlock& rhs) = default;

		SharedPtrControlBlock(SharedPtrControlBlock&& rhs) noexcept = default;
		SharedPtrControlBlock& operator=(SharedPtrControlBlock&& rhs) noexcept = default;

		DataType& GetManagedData() const;

		void IncrementStrongCount();
		std::uint32_t DecrementStrongCount();

		bool IncrementStrongCountIfNotZero();

		void IncrementWeakCount(const std::uint32_t adjustment = 1);
		std::uint32_t DecrementWeakCount();

	private:
		/// <summary>
		/// DataType is declared mutable so that SharedPtrControlBlock::GetManagedData() can
		/// be a const member function. This allows Brawler::SharedPtr::Get(), as well as its
		/// operator*() and operator->() overloads, to be const member functions.
		/// 
		/// We make those const member functions because std::shared_ptr::get() and its relevant
		/// operator overloads are also const member functions, despite granting non-const access
		/// to the managed data.
		/// </summary>
		mutable DataType mData;
		std::atomic<std::uint32_t> mStrongUseCount;
		std::atomic<std::uint32_t> mWeakUseCount;
	};
}

// -------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename DataType>
	template <typename... Args>
		requires std::constructible_from<DataType, Args...>
	SharedPtrControlBlock<DataType>::SharedPtrControlBlock(Args&&... args) :
		mData(std::forward<Args>(args)...),
		mStrongUseCount(0),
		mWeakUseCount(0)
	{}

	template <typename DataType>
	DataType& SharedPtrControlBlock<DataType>::GetManagedData() const
	{
		return mData;
	}

	template <typename DataType>
	void SharedPtrControlBlock<DataType>::IncrementStrongCount()
	{
		mStrongUseCount.fetch_add(1, std::memory_order::relaxed);
	}

	template <typename DataType>
	std::uint32_t SharedPtrControlBlock<DataType>::DecrementStrongCount()
	{
		// Use acquire-release memory ordering. We know that Brawler::SharedPtr
		// does not provide thread-safe access to the pointed-to object, so that is
		// not why we need it. Rather, we need this memory ordering because the thread
		// which calls the destructor of the pointed-to object *MUST* be able to
		// see the correct value of this object.
		// 
		// We expect the destructor to be called after decrementing the weak count
		// and finding that its new value is 0. However, we can only get the memory
		// ordering guarantees if we acquire on the same atomic variable which we
		// release. Thus, we need to use acquire-release memory ordering on mStrongUseCount;
		// release ordering on the decrement of mStrongUseCount followed by acquire
		// ordering on the decrement of mWeakUseCount will *NOT* work.
		//
		// The Stack Overflow answer at https://stackoverflow.com/a/48148318 is probably
		// the best explanation for this requirement which one could find.
		const std::uint32_t prevStrongCount = mStrongUseCount.fetch_sub(1, std::memory_order::acq_rel);

		if ((prevStrongCount - 1) == 0 && mWeakUseCount.load(std::memory_order::relaxed) == 0)
			delete this;

		return prevStrongCount;
	}

	template <typename DataType>
	bool SharedPtrControlBlock<DataType>::IncrementStrongCountIfNotZero()
	{
		while (true)
		{
			std::uint32_t currStrongCount = mStrongUseCount.load(std::memory_order::relaxed);

			if (currStrongCount == 0)
				return false;

			const std::uint32_t desiredStrongCount = (currStrongCount + 1);

			if (mStrongUseCount.compare_exchange_strong(currStrongCount, desiredStrongCount, std::memory_order::relaxed))
				return true;
		}
	}

	template <typename DataType>
	void SharedPtrControlBlock<DataType>::IncrementWeakCount(const std::uint32_t adjustment)
	{
		mWeakUseCount.fetch_add(adjustment, std::memory_order::relaxed);
	}

	template <typename DataType>
	std::uint32_t SharedPtrControlBlock<DataType>::DecrementWeakCount()
	{
		const std::uint32_t prevWeakCount = mWeakUseCount.fetch_sub(1, std::memory_order::acq_rel);

		if ((prevWeakCount - 1) == 0 && mStrongUseCount.load(std::memory_order::relaxed) == 0)
			delete this;

		return prevWeakCount;
	}
}
module;
#include <vector>
#include <cstddef>
#include <cassert>

export module Brawler.ThreadSafeVector;
import Brawler.CriticalSection;

export namespace Brawler
{
	template <typename T>
	class ThreadSafeVector
	{
	public:
		explicit ThreadSafeVector(const std::size_t numElementsToReserve = 0);

		ThreadSafeVector(const ThreadSafeVector& rhs);
		ThreadSafeVector& operator=(const ThreadSafeVector& rhs) = delete;

		ThreadSafeVector(ThreadSafeVector&& rhs) noexcept;
		ThreadSafeVector& operator=(ThreadSafeVector&& rhs) noexcept = delete;

		void PushBack(T&& val);
		bool IsLockFree() const;
		void Reserve(const std::size_t newCapacity);
		std::size_t GetSize() const;

		T& operator[](const std::size_t index);
		const T& operator[](const std::size_t index) const;

	private:
		std::vector<T> mArr;
		mutable CriticalSection mCritSection;
	};
}

// ----------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
	ThreadSafeVector<T>::ThreadSafeVector(const std::size_t numElementsToReserve) :
		mArr(),
		mCritSection()
	{
		mArr.reserve(numElementsToReserve);
	}

	template <typename T>
	ThreadSafeVector<T>::ThreadSafeVector(const ThreadSafeVector& rhs) :
		mCritSection()
	{
		Brawler::ScopedLock<CriticalSection> lock{ rhs.mCritSection };
		mArr = rhs.mArr;
	}

	template <typename T>
	ThreadSafeVector<T>::ThreadSafeVector(ThreadSafeVector&& rhs) noexcept :
		mCritSection()
	{
		Brawler::ScopedLock<CriticalSection> lock{ rhs.mCritSection };
		mArr = std::move(rhs.mArr);
	}

	template <typename T>
	void ThreadSafeVector<T>::PushBack(T&& val)
	{
		Brawler::ScopedLock<CriticalSection> lock{ mCritSection };
		mArr.push_back(std::forward<T>(val));
	}

	template <typename T>
	bool ThreadSafeVector<T>::IsLockFree() const
	{
		return false;
	}

	template <typename T>
	void ThreadSafeVector<T>::Reserve(const std::size_t newCapacity)
	{
		Brawler::ScopedLock<CriticalSection> lock{ mCritSection };
		mArr.reserve(newCapacity);
	}

	template <typename T>
	std::size_t ThreadSafeVector<T>::GetSize() const
	{
		Brawler::ScopedLock<CriticalSection> lock{ mCritSection };
		return mArr.size();
	}

	template <typename T>
	T& ThreadSafeVector<T>::operator[](const std::size_t index)
	{
		Brawler::ScopedLock<CriticalSection> lock{ mCritSection };
		
		assert(index < mArr.size() && "ERROR: An attempt was made to access a ThreadSafeVector, but the index passed to operator[]() went out of bounds!");
		return mArr[index];
	}

	template <typename T>
	const T& ThreadSafeVector<T>::operator[](const std::size_t index) const
	{
		Brawler::ScopedLock<CriticalSection> lock{ mCritSection };

		assert(index < mArr.size() && "ERROR: An attempt was made to access a ThreadSafeVector, but the index passed to operator[]() went out of bounds!");
		return mArr[index];
	}
}
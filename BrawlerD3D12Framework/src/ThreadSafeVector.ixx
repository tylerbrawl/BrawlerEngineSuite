module;
#include <mutex>
#include <shared_mutex>
#include <ranges>
#include <algorithm>
#include <cassert>

export module Brawler.ThreadSafeVector;
import Brawler.ScopedSharedLock;
import Brawler.Functional;

namespace Brawler
{
	template <typename LockType>
	concept Lockable = requires (LockType x)
	{
		x.lock();
		x.unlock();
	};

	template <typename T>
	struct LockInfo
	{
		static_assert(sizeof(T) != sizof(T));
	};

	template <typename ScopedReadLockType_, typename ScopedWriteLockType_>
	struct LockInfoInstantiation
	{
		using ScopedReadLockType = ScopedReadLockType_;
		using ScopedWriteLockType = ScopedWriteLockType_;
	};

	template <>
	struct LockInfo<std::mutex> : public LockInfoInstantiation<std::scoped_lock<std::mutex>, std::scoped_lock<std::mutex>>
	{};

	template <>
	struct LockInfo<std::shared_mutex> : public LockInfoInstantiation<ScopedSharedReadLock<std::shared_mutex>, ScopedSharedWriteLock<std::shared_mutex>>
	{};
}

export namespace Brawler
{
	template <typename T, typename LockType = std::mutex>
		requires Lockable<LockType>
	class ThreadSafeVector
	{
	private:
		using ScopedReadLockType = typename LockInfo<LockType>::ScopedReadLockType;
		using ScopedWriteLockType = typename LockInfo<LockType>::ScopedWriteLockType;

	public:
		ThreadSafeVector() = default;

		template <typename RHSLockType>
		ThreadSafeVector(const ThreadSafeVector<T, RHSLockType>& rhs);

		template <typename RHSLockType>
		ThreadSafeVector& operator=(const ThreadSafeVector<T, RHSLockType>& rhs);

		template <typename RHSLockType>
		ThreadSafeVector(ThreadSafeVector<T, RHSLockType>&& rhs) noexcept;

		template <typename RHSLockType>
		ThreadSafeVector& operator=(ThreadSafeVector<T, RHSLockType>&& rhs) noexcept;

		template <typename U>
			requires std::is_same_v<std::decay_t<T>, std::decay_t<U>>
		void PushBack(U&& val);

		template <typename... Args>
			requires requires (Args... args)
		{
			T{ args... };
		}
		void EmplaceBack(Args&&... args);

		void Reserve(const std::size_t desiredCapacity);
		std::size_t GetSize() const;
		bool Empty() const;

		void Erase(const std::size_t index);

		template <typename Callback>
		void EraseIf(const Callback& predicate);

		void Clear();

		template <typename Callback>
		void AccessData(const std::size_t index, const Callback& callback) const;

		template <typename Callback>
		void ForEach(const Callback& callback);

		template <typename Callback>
		void ForEach(const Callback& callback) const;

		/// <summary>
		/// Seriously? Do you really need to ask? I mean, LockType is one of the template
		/// parameters, for God's sake.
		/// </summary>
		/// <returns>
		/// What do you think?
		/// </returns>
		constexpr static bool IsLockFree();

	private:
		std::vector<T> mDataArr;
		mutable LockType mCritSection;
	};
}

// --------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T, typename LockType>
		requires Lockable<LockType>
	template <typename RHSLockType>
	ThreadSafeVector<T, LockType>::ThreadSafeVector(const ThreadSafeVector<T, RHSLockType>& rhs) :
		mDataArr(),
		mCritSection()
	{
		typename LockInfo<RHSLockType>::ScopedReadLockType readLock{ rhs.mCritSection };

		mDataArr = rhs.mDataArr;
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	template <typename RHSLockType>
	ThreadSafeVector<T, LockType>& ThreadSafeVector<T, LockType>::operator=(const ThreadSafeVector<T, RHSLockType>& rhs)
	{
		ScopedWriteLockType lhsWriteLock{ mCritSection };
		typename LockInfo<RHSLockType>::ScopedReadLockType rhsReadLock{ rhs.mCritSection };

		mDataArr = rhs.mDataArr;

		return *this;
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	template <typename RHSLockType>
	ThreadSafeVector<T, LockType>::ThreadSafeVector(ThreadSafeVector<T, RHSLockType>&& rhs) noexcept :
		mDataArr(),
		mCritSection()
	{
		typename LockInfo<RHSLockType>::ScopedWriteLockType rhsWriteLock{ rhs.mCritSection };

		mDataArr = std::move(rhs.mDataArr);
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	template <typename RHSLockType>
	ThreadSafeVector<T, LockType>& ThreadSafeVector<T, LockType>::operator=(ThreadSafeVector<T, RHSLockType>&& rhs) noexcept
	{
		ScopedWriteLockType lhsWriteLock{ mCritSection };
		typename LockInfo<RHSLockType>::ScopedWriteLockType rhsWriteLock{ rhs.mCritSection };

		mDataArr = std::move(rhs.mDataArr);

		return *this;
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	template <typename U>
		requires std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	void ThreadSafeVector<T, LockType>::PushBack(U&& val)
	{
		ScopedWriteLockType writeLock{ mCritSection };

		mDataArr.push_back(std::forward<U>(val));
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	template <typename... Args>
		requires requires (Args... args)
	{
		T{ args... };
	}
	void ThreadSafeVector<T, LockType>::EmplaceBack(Args&&... args)
	{
		ScopedWriteLockType writeLock{ mCritSection };

		mDataArr.emplace_back(std::forward<Args>(args)...);
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	void ThreadSafeVector<T, LockType>::Reserve(const std::size_t desiredCapacity)
	{
		ScopedWriteLockType writeLock{ mCritSection };

		mDataArr.reserve(desiredCapacity);
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	std::size_t ThreadSafeVector<T, LockType>::GetSize() const
	{
		ScopedReadLockType readLock{ mCritSection };

		return mDataArr.size();
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	bool ThreadSafeVector<T, LockType>::Empty() const
	{
		ScopedReadLockType readLock{ mCritSection };

		return mDataArr.empty();
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	void ThreadSafeVector<T, LockType>::Erase(const std::size_t index)
	{
		ScopedWriteLockType writeLock{ mCritSection };

		assert(index < mDataArr.size() && "ERROR: An out-of-bounds index was provided to ThreadSafeVector::Erase()!");
		mDataArr.erase(mDataArr.begin() + index);
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	template <typename Callback>
	void ThreadSafeVector<T, LockType>::EraseIf(const Callback& predicate)
	{
		ScopedWriteLockType writeLock{ mCritSection };

		std::erase_if(mDataArr, predicate);
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	void ThreadSafeVector<T, LockType>::Clear()
	{
		ScopedWriteLockType writeLock{ mCritSection };

		mDataArr.clear();
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	template <typename Callback>
	void ThreadSafeVector<T, LockType>::AccessData(const std::size_t index, const Callback& callback) const
	{
		ScopedReadLockType readLock{ mCritSection };

		assert(index < mDataArr.size() && "ERROR: An out-of-bounds index was provided to ThreadSafeVector::AccessData()!");
		callback(mDataArr[index]);
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	template <typename Callback>
	void ThreadSafeVector<T, LockType>::ForEach(const Callback& callback)
	{
		ScopedReadLockType readLock{ mCritSection };

		std::ranges::for_each(mDataArr, callback);
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	template <typename Callback>
	void ThreadSafeVector<T, LockType>::ForEach(const Callback& callback) const
	{
		ScopedReadLockType readLock{ mCritSection };

		std::ranges::for_each(mDataArr, callback);
	}

	template <typename T, typename LockType>
		requires Lockable<LockType>
	constexpr static bool ThreadSafeVector<T, LockType>::IsLockFree()
	{
		return false;
	}
}
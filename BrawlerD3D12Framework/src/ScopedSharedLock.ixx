module;
#include <mutex>

export module Brawler.ScopedSharedLock;

namespace Brawler
{
	// This is the std-defined SharedMutex concept (see https://en.cppreference.com/w/cpp/named_req/SharedMutex).
	template <typename T>
	concept SharedMutex = requires (T x)
	{
		x.lock_shared();
		x.try_lock_shared();
		x.unlock_shared();
	};
}

export namespace Brawler
{
	template <typename... T>
	using ScopedSharedWriteLock = std::scoped_lock<T...>;

	template <typename... T>
		requires (SharedMutex<T> && ...)
	class ScopedSharedReadLock
	{
	public:
		explicit ScopedSharedReadLock(T&... critSections);
		~ScopedSharedReadLock();

	private:
		template <std::size_t CurrIndex>
		void UnlockCriticalSections();

	private:
		std::tuple<T&...> mCritSectionTuple;
	};
}

// ----------------------------------------------------------------------------------

namespace Brawler
{
	template <typename... T>
		requires (SharedMutex<T> && ...)
	ScopedSharedReadLock<T...>::ScopedSharedReadLock(T&... critSections) :
		mCritSectionTuple(std::tie(critSections...))
	{
		(critSections.lock_shared(), ...);
	}

	template <typename... T>
		requires (SharedMutex<T> && ...)
	ScopedSharedReadLock<T...>::~ScopedSharedReadLock()
	{
		UnlockCriticalSections<0>();
	}

	template <typename... T>
		requires (SharedMutex<T> && ...)
	template <std::size_t CurrIndex>
	void ScopedSharedReadLock<T...>::UnlockCriticalSections()
	{
		// Since we lock the critical sections in the order 1 -> 2 -> 3 -> ...,
		// we need to unlock them in the opposite order, i.e., ... -> 3 -> 2 -> 1.
		
		if constexpr ((CurrIndex + 1) < std::tuple_size_v<decltype(mCritSectionTuple)>)
			UnlockCriticalSections<(CurrIndex + 1)>();

		std::get<CurrIndex>(mCritSectionTuple).unlock_shared();
	}
}
module;
#include <mutex>

export module Brawler.CriticalSection;

export namespace Brawler
{
	using CriticalSection = std::mutex;
	
	template <typename T>
	using ScopedLock = std::scoped_lock<T>;
}
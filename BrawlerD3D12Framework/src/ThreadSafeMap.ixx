module;
#include <functional>
#include <array>
#include <cassert>

export module Brawler.ThreadSafeMap;
import Brawler.Functional;

export namespace Brawler
{
	template <typename Key, typename Value, std::size_t NumElements>
	class ThreadSafeMap
	{
	public:
		ThreadSafeMap() = default;

		ThreadSafeMap(const ThreadSafeMap& rhs) = delete;
		ThreadSafeMap& operator=(const ThreadSafeMap& rhs) = delete;

		ThreadSafeMap(ThreadSafeMap&& rhs) noexcept = delete;
		ThreadSafeMap& operator=(ThreadSafeMap&& rhs) noexcept = delete;

		Value& operator[](const Key& key);
		
		Value& At(const Key& key);
		const Value& At(const Key& key) const;

		/// <summary>
		/// Removes the element identified by the Key value key from the ThreadSafeMap instance.
		/// Doing this will free up space for other elements to be added to the map.
		/// 
		/// NOTE: It is the caller's responsibility to ensure that no other threads are accessing
		/// the data or will access the data after the element is removed.
		/// </summary>
		/// <param name="key">
		/// - The key which identifies the element to remove from the ThreadSafeMap instance.
		/// </param>
		void Erase(const Key& key);

		/// <summary>
		/// Removes all of the elements from the ThreadSafeMap instance. Doing this will free up
		/// space for other elements to be added to the map.
		/// 
		/// NOTE: It is the caller's responsibility to ensure that no other threads are accessing
		/// the map while it is being cleared, and that threads do not expect old data to be
		/// present following a call to this function. To be perfectly clear, it is also illegal
		/// to access the data elements within a ThreadSafeMap after this function is called
		/// until these elements have been initialized again.
		/// </summary>
		void Clear();

	private:
		std::array<Value, NumElements> mDataArr;
		std::array<std::atomic<std::size_t>, NumElements> mKeyHashArr;
	};
}

// ----------------------------------------------------------------------------------------------------------------------

namespace
{
	// We need a value which can be used to distinguish used slots in the ThreadSafeMap
	// from old ones. Since the values in the hash array are zero initialized anyways,
	// we may as well make this value 0.
	static constexpr std::size_t INVALID_HASH_KEY = 0;

	template <typename Key>
	std::size_t CalculateHash(const Key& key)
	{
		const std::hash<Key> hasher{};
		std::size_t keyHash = hasher(key);

		// Prevent the hash from being the same as INVALID_HASH_KEY. This is unlikely
		// to happen, but for correctness, we need to do it anyways.
		if (keyHash == INVALID_HASH_KEY) [[unlikely]]
			--keyHash;

		return keyHash;
	}
}

namespace Brawler
{
	template <typename Key, typename Value, std::size_t NumElements>
	Value& ThreadSafeMap<Key, Value, NumElements>::operator[](const Key& key)
	{
		const std::size_t keyHash = CalculateHash(key);

		std::size_t currIndex = (keyHash % NumElements);
		Value* foundValue = nullptr;
		std::size_t numElementsChecked = 0;

		while (numElementsChecked < NumElements)
		{
			bool reTryElement = false;
			const std::size_t currHash = mKeyHashArr[currIndex].load();

			if (currHash == INVALID_HASH_KEY)
			{
				std::size_t expectedHash = INVALID_HASH_KEY;
				if (mKeyHashArr[currIndex].compare_exchange_strong(expectedHash, keyHash))
				{
					foundValue = &(mDataArr[currIndex]);
					break;
				}
				else
				{
					// If we find that the hash value has changed, then it is possible that
					// another thread has added the value corresponding to this key to the
					// map. In that case, we need to re-try the element to ensure that we
					// do not create duplicates.
					reTryElement = true;
				}
					
			}
			else if (currHash == keyHash)
			{
				foundValue = &(mDataArr[currIndex]);
				break;
			}

			if (!reTryElement)
			{
				++numElementsChecked;
				currIndex = ((currIndex + 1) % NumElements);
			}
		}

		assert(numElementsChecked != NumElements && "ERROR: There was no storage left to allocate an additional element in a ThreadSafeMap! Consider either deleting unused elements or increasing its capacity.");
		return *foundValue;
	}

	template <typename Key, typename Value, std::size_t NumElements>
	Value& ThreadSafeMap<Key, Value, NumElements>::At(const Key& key)
	{
		const std::size_t keyHash = CalculateHash(key);
		std::size_t currIndex = (keyHash % NumElements);
		Value* foundValue = nullptr;
		std::size_t numElementsChecked = 0;

		while (numElementsChecked < NumElements)
		{
			const std::size_t currHash = mKeyHashArr[currIndex].load();

			if (currHash == keyHash)
			{
				foundValue = &(mDataArr[currIndex]);
				break;
			}

			++numElementsChecked;
			currIndex = ((currIndex + 1) % NumElements);
		}

		assert(numElementsChecked != NumElements && "ERROR: An attempt was made to call ThreadSafeMap::At() using a key which did not already exist!");
		return *foundValue;
	}

	template <typename Key, typename Value, std::size_t NumElements>
	const Value& ThreadSafeMap<Key, Value, NumElements>::At(const Key& key) const
	{
		const std::size_t keyHash = CalculateHash(key);
		std::size_t currIndex = (keyHash % NumElements);
		const Value* foundValue = nullptr;
		std::size_t numElementsChecked = 0;

		while (numElementsChecked < NumElements)
		{
			const std::size_t currHash = mKeyHashArr[currIndex].load();

			if (currHash == keyHash)
			{
				foundValue = &(mDataArr[currIndex]);
				break;
			}

			++numElementsChecked;
			currIndex = ((currIndex + 1) % NumElements);
		}

		assert(numElementsChecked != NumElements && "ERROR: An attempt was made to call ThreadSafeMap::At() using a key which did not already exist!");
		return *foundValue;
	}

	template <typename Key, typename Value, std::size_t NumElements>
	void ThreadSafeMap<Key, Value, NumElements>::Erase(const Key& key)
	{
		const std::size_t keyHash = CalculateHash(key);
		std::size_t currIndex = (keyHash % NumElements);
		std::size_t numElementsChecked = 0;
		std::size_t expectedHash = keyHash;

		while (numElementsChecked < NumElements)
		{
			std::size_t currHash = mKeyHashArr[currIndex].load();

			// If we find an unoccupied slot, then we can conclude that no future slots are
			// occupied with the same key, so we can exit early.
			if (currHash == INVALID_HASH_KEY)
				return;
			
			else if (currHash == keyHash && mKeyHashArr[currIndex].compare_exchange_strong(currHash, INVALID_HASH_KEY))
				return;

			expectedHash = keyHash;
			++numElementsChecked;
			currIndex = ((currIndex + 1) % NumElements);
		}
	}

	template <typename Key, typename Value, std::size_t NumElements>
	void ThreadSafeMap<Key, Value, NumElements>::Clear()
	{
		for (auto& hash : mKeyHashArr)
			hash.store(INVALID_HASH_KEY);
	}
}
module;
#include <vector>
#include <limits>
#include <optional>
#include <cassert>

export module Brawler.FastUnorderedMap;
import Brawler.Functional;

export namespace Brawler
{
	template <typename Key, typename Value, typename Hasher = std::hash<Key>>
	class FastUnorderedMap
	{
	private:
		struct MapElement
		{
			std::optional<Value> Data;
			std::size_t KeyHash;
		};

	public:
		explicit constexpr FastUnorderedMap(const std::size_t expectedSize = 0);

		// Although there is technically nothing wrong with allowing copying of
		// FastUnorderedMap instances, we want this class to be *fast*, and copying is
		// inherently slow.

		FastUnorderedMap(const FastUnorderedMap& rhs) = delete;
		FastUnorderedMap& operator=(const FastUnorderedMap& rhs) = delete;

		constexpr FastUnorderedMap(FastUnorderedMap&& rhs) noexcept = default;
		constexpr FastUnorderedMap& operator=(FastUnorderedMap&& rhs) noexcept = default;

		template <typename... Args>
			requires requires (Args&&... args)
		{
			Value{ std::forward<Args>(args)... };
		}
		constexpr bool TryEmplace(const Key& key, Args&&... args);

		constexpr Value& operator[](const Key& key) requires std::is_default_constructible_v<Value>;

		constexpr Value& At(const Key& key);
		constexpr const Value& At(const Key& key) const;

		constexpr bool Contains(const Key& key) const;

		/// <summary>
		/// Reserves exactly newCapacity slots in the underlying bucket array, but only if
		/// newCapacity is greater than the size of said bucket array.
		/// 
		/// Since this reservation is exact, you might benefit from increasing it slightly
		/// past what you might expect. For instance, if you are expecting the map to contain
		/// X elements created from pointer hashes, you might want to specify (X * 2) as the
		/// value for newCapacity. Doing this can help reduce the load factor and thus reduce
		/// the chances of a re-hash occurring.
		/// 
		/// *NOTE*: This function can trigger a re-hash. After calling it, all cached pointers
		/// to elements in the FastUnorderedMap instance should be considered invalid.
		/// </summary>
		/// <param name="newCapacity">
		/// - The size, in elements, of the underlying bucket array.
		/// </param>
		constexpr void Reserve(const std::size_t newCapacity);

		template <Brawler::Function<void, Value&> Callback>
		constexpr void ForEach(const Callback& callback);

		template <Brawler::Function<void, const Value&> Callback>
		constexpr void ForEach(const Callback& callback) const;

	private:
		/// <summary>
		/// Doubles the size of the underlying array to allow for additional entries. Doing
		/// this will likely result in shifting element locations.
		/// </summary>
		constexpr void ReHash();

		constexpr void ReHash(const std::size_t newSize);

		constexpr float CalculateLoadFactor() const;

	private:
		std::vector<MapElement> mElementArr;
		std::size_t mNumExistingElements;
		inline static Hasher mHasher = Hasher{};
	};
}

// ---------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	static constexpr float LOAD_FACTOR_REHASH_THRESHOLD = 0.7f;
}

namespace Brawler
{
	template <typename Key, typename Value, typename Hasher>
	constexpr FastUnorderedMap<Key, Value, Hasher>::FastUnorderedMap(const std::size_t expectedSize) :
		mElementArr(),
		mNumExistingElements(0)
	{
		mElementArr.resize(std::max<std::size_t>(expectedSize, 1));
	}

	template <typename Key, typename Value, typename Hasher>
	template <typename... Args>
		requires requires (Args&&... args)
	{
		Value{ std::forward<Args>(args)... };
	}
	constexpr bool FastUnorderedMap<Key, Value, Hasher>::TryEmplace(const Key& key, Args&&... args)
	{
		const std::size_t keyHash = mHasher(key);

		if (CalculateLoadFactor() >= LOAD_FACTOR_REHASH_THRESHOLD) [[unlikely]]
			ReHash();
		
		while (true)
		{
			std::size_t currIndex = (keyHash % mElementArr.size());

			for (std::size_t numIterations = 0; numIterations < mElementArr.size(); ++numIterations)
			{
				MapElement& currElement{ mElementArr[currIndex] };

				if (!currElement.Data.has_value())
				{
					currElement.Data.emplace(std::forward<Args>(args)...);
					currElement.KeyHash = keyHash;

					++mNumExistingElements;

					return true;
				}

				else if (currElement.KeyHash == keyHash)
					return false;

				currIndex = ((currIndex + 1) % mElementArr.size());
			}

			ReHash();
		}
	}

	template <typename Key, typename Value, typename Hasher>
	constexpr Value& FastUnorderedMap<Key, Value, Hasher>::operator[](const Key& key) requires std::is_default_constructible_v<Value>
	{
		const std::size_t keyHash = mHasher(key);
		std::size_t currIndex = (keyHash % mElementArr.size());

		// First, try finding an existing value in the map.
		for (std::size_t numIterations = 0; numIterations < mElementArr.size(); ++numIterations)
		{
			MapElement& currElement{ mElementArr[currIndex] };

			if (currElement.Data.has_value() && currElement.KeyHash == keyHash)
				return *(currElement.Data);

			else if (!currElement.Data.has_value())
				break;

			currIndex = ((currIndex + 1) % mElementArr.size());
		}

		// If that failed, then we construct the element.
		{
			const bool tryEmplaceResult = TryEmplace(key);
			assert(tryEmplaceResult);
		}

		// At this point, the element should definitely be constructed.
		return At(key);
	}

	template <typename Key, typename Value, typename Hasher>
	constexpr Value& FastUnorderedMap<Key, Value, Hasher>::At(const Key& key)
	{
		assert(Contains(key) && "ERROR: FastUnorderedMap::At() was called to retrieve an existing element associated with a given key, but no such element exists!");
		
		const std::size_t keyHash = mHasher(key);
		std::size_t currIndex = (keyHash % mElementArr.size());

		for (std::size_t numIterations = 0; numIterations < mElementArr.size(); ++numIterations)
		{
			MapElement& currElement{ mElementArr[currIndex] };

			// We add [[likely]] here because average lookup time is O(1), and because when we
			// call FastUnorderedMap::At(), we expect the element to exist.
			if (currElement.Data.has_value() && currElement.KeyHash == keyHash) [[likely]]
				return *(currElement.Data);

			else if (!currElement.Data.has_value())
				break;

			currIndex = ((currIndex + 1) % mElementArr.size());
		}

		// Since we assert that the map contains the specified key, this should be unreachable.
		std::unreachable();

		// Well, we've already invoked undefined behavior by calling std::unreachable(), so...
		return reinterpret_cast<Value&>(*this);
	}

	template <typename Key, typename Value, typename Hasher>
	constexpr const Value& FastUnorderedMap<Key, Value, Hasher>::At(const Key& key) const
	{
		assert(Contains(key) && "ERROR: FastUnorderedMap::At() was called to retrieve an existing element associated with a given key, but no such element exists!");

		const std::size_t keyHash = mHasher(key);
		std::size_t currIndex = (keyHash % mElementArr.size());

		for (std::size_t numIterations = 0; numIterations < mElementArr.size(); ++numIterations)
		{
			MapElement& currElement{ mElementArr[currIndex] };

			// We add [[likely]] here because average lookup time is O(1), and because when we
			// call FastUnorderedMap::At(), we expect the element to exist.
			if (currElement.Data.has_value() && currElement.KeyHash == keyHash) [[likely]]
				return *(currElement.Data);

			else if (!currElement.Data.has_value())
				break;

			currIndex = ((currIndex + 1) % mElementArr.size());
		}

		// Since we assert that the map contains the specified key, this should be unreachable.
		std::unreachable();

		// Well, we've already invoked undefined behavior by calling std::unreachable(), so...
		return reinterpret_cast<const Value&>(*this);
	}

	template <typename Key, typename Value, typename Hasher>
	constexpr bool FastUnorderedMap<Key, Value, Hasher>::Contains(const Key& key) const
	{
		const std::size_t keyHash = mHasher(key);
		std::size_t currIndex = (keyHash % mElementArr.size());

		for (std::size_t numIterations = 0; numIterations < mElementArr.size(); ++numIterations)
		{
			const MapElement& currElement{ mElementArr[currIndex] };

			if (currElement.Data.has_value() && currElement.KeyHash == keyHash)
				return true;

			else if (!currElement.Data.has_value())
				return false;

			currIndex = ((currIndex + 1) % mElementArr.size());
		}

		return false;
	}

	template <typename Key, typename Value, typename Hasher>
	constexpr void FastUnorderedMap<Key, Value, Hasher>::Reserve(const std::size_t newCapacity)
	{
		if (newCapacity <= mElementArr.size())
			return;

		ReHash(newCapacity);
	}

	template <typename Key, typename Value, typename Hasher>
	template <Brawler::Function<void, Value&> Callback>
	constexpr void FastUnorderedMap<Key, Value, Hasher>::ForEach(const Callback& callback)
	{
		for (auto& element : mElementArr)
		{
			if (element.Data.has_value())
				callback(*(element.Data));
		}
	}

	template <typename Key, typename Value, typename Hasher>
	template <Brawler::Function<void, const Value&> Callback>
	constexpr void FastUnorderedMap<Key, Value, Hasher>::ForEach(const Callback& callback) const
	{
		for (const auto& element : mElementArr)
		{
			if (element.Data.has_value())
				callback(*(element.Data));
		}
	}

	template <typename Key, typename Value, typename Hasher>
	constexpr void FastUnorderedMap<Key, Value, Hasher>::ReHash()
	{
		ReHash(mElementArr.size() * 2);
	}

	template <typename Key, typename Value, typename Hasher>
	constexpr void FastUnorderedMap<Key, Value, Hasher>::ReHash(const std::size_t newSize)
	{
		assert(newSize > mElementArr.size());
		std::size_t currExpandedSize = newSize;
		bool executeReHash = true;

		while (executeReHash)
		{
			executeReHash = false;
			
			std::vector<MapElement> newElementArr{};
			newElementArr.resize(currExpandedSize);

			for (auto&& element : mElementArr)
			{
				if (!element.Data.has_value())
					continue;

				std::size_t currIndex = (element.KeyHash % newElementArr.size());
				std::size_t numIterations = 0;

				while (newElementArr[currIndex].Data.has_value() && numIterations < newElementArr.size())
				{
					currIndex = ((currIndex + 1) % newElementArr.size());
					++numIterations;
				}

				if (numIterations == newElementArr.size()) [[unlikely]]
				{
					executeReHash = true;
					break;
				}
				else [[likely]]
				{
					assert(!(newElementArr[currIndex].Data.has_value()));
					newElementArr[currIndex] = std::move(element);
				}
			}

			if (executeReHash)
				currExpandedSize *= 2;
			else
				mElementArr = std::move(newElementArr);
		}
	}

	template <typename Key, typename Value, typename Hasher>
	constexpr float FastUnorderedMap<Key, Value, Hasher>::CalculateLoadFactor() const
	{
		return (static_cast<float>(mNumExistingElements) / static_cast<float>(mElementArr.size()));
	}
}
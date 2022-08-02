module;
#include <array>
#include <cassert>
#include <atomic>
#include <mutex>
#include <optional>

export module Brawler.ThreadSafeUnorderedMap;
import Brawler.SharedPtr;
import Brawler.Functional;

namespace Brawler
{
	// The idea for this ThreadSafeUnorderedMap comes from the paper "A Wait-Free Hash Map" from
	// Laborde, Feldman, and Dechev. Unlike their implementation, this one is not wait free.

	static constexpr std::size_t BASE_ARRAY_LENGTH = 16;
	static_assert(std::popcount(BASE_ARRAY_LENGTH) == 1, "ERROR: The value BASE_ARRAY_LENGTH for Brawler::ThreadSafeUnorderedMap *MUST* be a power of two!");

	static constexpr std::size_t BASE_ARRAY_HASH_BITMASK = (BASE_ARRAY_LENGTH - 1);

	static constexpr std::size_t ARRAY_NODE_ARRAY_LENGTH = 16;
	static_assert(std::popcount(ARRAY_NODE_ARRAY_LENGTH) == 1, "ERROR: The value ARRAY_NODE_ARRAY_LENGTH for Brawler::ThreadSafeUnorderedMap *MUST* be a power of two!");

	static constexpr std::size_t ARRAY_NODE_HASH_BITMASK = (ARRAY_NODE_ARRAY_LENGTH - 1);

	static constexpr std::size_t HASH_BIT_SHIFT_AMOUNT_PER_LEVEL = std::countr_zero(ARRAY_NODE_ARRAY_LENGTH);

	struct LockDisabler
	{};

	template <typename LockType>
	concept IsLockable = std::is_default_constructible_v<LockType> && requires (LockType& critSection)
	{
		std::scoped_lock<LockType>{ critSection };
	};

	template <typename Value, typename LockType>
		requires IsLockable<LockType>
	class ValueContainer
	{
	public:
		ValueContainer() = default;

		template <typename... Args>
			requires std::constructible_from<Value, Args...>
		ValueContainer(Args&&... args) :
			mData(std::in_place, std::forward<Args>(args)...),
			mCritSection()
		{}

		ValueContainer(const ValueContainer& rhs) = default;
		ValueContainer& operator=(const ValueContainer& rhs) = default;

		ValueContainer(ValueContainer&& rhs) noexcept = default;
		ValueContainer& operator=(ValueContainer&& rhs) noexcept = default;

		template <typename Callback>
		void AccessData(const Callback& callback)
		{
			std::scoped_lock<LockType> lock{ mCritSection };
			callback(*mData);
		}

		static consteval bool IsLockFree()
		{
			return false;
		}

	private:
		std::optional<Value> mData;
		mutable LockType mCritSection;
	};

	template <typename Value>
	class ValueContainer<Value, LockDisabler>
	{
	public:
		ValueContainer() = default;

		template <typename... Args>
			requires std::constructible_from<Value, Args...>
		ValueContainer(Args&&... args) :
			mData(std::in_place, std::forward<Args>(args)...)
		{}

		ValueContainer(const ValueContainer& rhs) = default;
		ValueContainer& operator=(const ValueContainer& rhs) = default;

		ValueContainer(ValueContainer&& rhs) noexcept = default;
		ValueContainer& operator=(ValueContainer&& rhs) noexcept = default;

		template <typename Callback>
		void AccessData(const Callback& callback)
		{
			callback(*mData);
		}

		static consteval bool IsLockFree()
		{
			return std::atomic<bool>::is_always_lock_free;
		}

	private:
		std::optional<Value> mData;
	};
}

export namespace Brawler
{
	template <typename Key, typename Value, typename LockType = LockDisabler, typename Hasher = std::hash<Key>>
	class ThreadSafeUnorderedMap
	{
	private:
		enum class NodeType : bool
		{
			DATA_NODE,
			ARRAY_NODE
		};

		struct MapNode
		{
			ValueContainer<Value, LockType> Data;
			std::size_t HashValue;
			std::array<Brawler::AtomicSharedPtr<MapNode>, ARRAY_NODE_ARRAY_LENGTH> ChildNodeArr;
			NodeType Type;

			template <typename... Args>
				requires std::constructible_from<Value, Args...>
			MapNode(Args&&... args) :
				Data(std::forward<Args>(args)...),
				HashValue(0),
				ChildNodeArr(),
				Type(NodeType::DATA_NODE)
			{}

			template <typename Callback>
			void ForEachIMPL(const Callback& callback)
			{
				if (Type == NodeType::DATA_NODE)
					Data.AccessData(callback);
				else
				{
					for (const auto& atomicChildNodePtr : ChildNodeArr)
					{
						const Brawler::SharedPtr<MapNode> childNodePtr{ atomicChildNodePtr.Load(std::memory_order::acquire) };

						if (childNodePtr != nullptr)
							childNodePtr->ForEachIMPL(callback);
					}
				}
			}
		};

	public:
		ThreadSafeUnorderedMap() = default;

		ThreadSafeUnorderedMap(const ThreadSafeUnorderedMap& rhs) = delete;
		ThreadSafeUnorderedMap& operator=(const ThreadSafeUnorderedMap& rhs) = delete;

		ThreadSafeUnorderedMap(ThreadSafeUnorderedMap&& rhs) noexcept = default;
		ThreadSafeUnorderedMap& operator=(ThreadSafeUnorderedMap&& rhs) noexcept = default;

		template <typename... Args>
			requires std::constructible_from<Value, Args...>
		bool TryEmplace(const Key& key, Args&&... args);

		// NOTE: std::unordered_map has two primary member functions for accessing data elements:
		//
		//   - std::unordered_map::at()
		//   - std::unordered_map::operator[]
		//
		// We cannot use these because they return a reference to a Value instance which could
		// potentially be destroyed by other threads concurrently.
		//
		// Instead, ThreadSafeUnorderedMap provides a different API in the form of
		// ThreadSafeUnorderedMap::AccessData(). This function searches for an element with the
		// specified key and, if such an element is found, executes a specified callback, passing
		// in the corresponding Value instance.
		//
		// In doing it this way, we are able to keep a SharedPtr on the stack and prevent deletion
		// while other threads are reading the data. This also allows us to easily lock access to
		// individual Value instances, should we choose to do so.

		template <Brawler::Function<void, Value&> Callback>
		bool AccessData(const Key& key, const Callback& callback);

		template <Brawler::Function<void, Value> Callback>
		bool AccessData(const Key& key, const Callback& callback) const;
		
		template <Brawler::Function<void, const Value&> Callback>
		bool AccessData(const Key& key, const Callback& callback) const;

		bool Contains(const Key& key) const;
		bool Empty() const;

		template <Brawler::Function<void, Value&> Callback>
		void ForEach(const Callback& callback);

		template <Brawler::Function<void, const Value&> Callback>
		void ForEach(const Callback& callback) const;

		bool Erase(const Key& key);
		void Clear();

		static consteval bool IsLockFree();

	private:
		Brawler::WeakPtr<MapNode> TryFindExistingNodeForKey(const Key& key) const;

	private:
		std::array<Brawler::AtomicSharedPtr<MapNode>, BASE_ARRAY_LENGTH> mHeadNodeArr;
	};
}

// ------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename Key, typename Value, typename LockType, typename Hasher>
	template <typename... Args>
		requires std::constructible_from<Value, Args...>
	bool ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::TryEmplace(const Key& key, Args&&... args)
	{
		const std::size_t keyHash = Hasher{}(key);

		// In order to prevent race conditions, we have to construct the data *BEFORE* inserting
		// it into the map. This is different behavior from std::unordered_map::try_emplace(),
		// which will only construct the object if no element for the specified key already exists.
		//
		// The alternative would be to lock the data, but this would result in a data structure
		// which is never really lock free.
		const Brawler::SharedPtr<MapNode> potentialInsertionNodePtr{ Brawler::MakeShared<MapNode>(std::forward<Args>(args)...) };
		potentialInsertionNodePtr->HashValue = keyHash;

		std::size_t currDepth = 0;
		Brawler::AtomicSharedPtr<MapNode>* currAtomicNodePtr = nullptr;
		Brawler::SharedPtr<MapNode> currHeadNodePtr{};

		while (true)
		{
			const std::size_t shiftedHash = (keyHash >> (HASH_BIT_SHIFT_AMOUNT_PER_LEVEL * currDepth));

			if (currDepth == 0)
				currAtomicNodePtr = &(mHeadNodeArr[shiftedHash & BASE_ARRAY_HASH_BITMASK]);
			else
				currAtomicNodePtr = &(currHeadNodePtr->ChildNodeArr[shiftedHash & ARRAY_NODE_HASH_BITMASK]);

			currHeadNodePtr = currAtomicNodePtr->Load(std::memory_order::acquire);

			if (currHeadNodePtr == nullptr)
			{
				if (currAtomicNodePtr->CompareExchangeStrong(currHeadNodePtr, potentialInsertionNodePtr, std::memory_order::release, std::memory_order::relaxed))
					return true;
			}
			else if (currHeadNodePtr->Type == NodeType::DATA_NODE)
			{
				if (currHeadNodePtr->HashValue == keyHash)
					return false;

				// Try to replace this node with an array node.
				const Brawler::SharedPtr<MapNode> replacementArrayNodePtr{ Brawler::MakeShared<MapNode>() };
				replacementArrayNodePtr->Type = NodeType::ARRAY_NODE;

				// Add the current node to its corresponding location in the replacement array node.
				const std::size_t shiftedCurrHeadNodeHash = (currHeadNodePtr->HashValue >> (HASH_BIT_SHIFT_AMOUNT_PER_LEVEL * (currDepth + 1)));
				const std::size_t newHeadNodeIndex = (shiftedCurrHeadNodeHash & ARRAY_NODE_HASH_BITMASK);
				replacementArrayNodePtr->ChildNodeArr[newHeadNodeIndex].Store(currHeadNodePtr, std::memory_order::relaxed);

				// As an optimization, we also try to add potentialInsertionNodePtr to the array node,
				// but only if it does not have the same index as the head node.
				const std::size_t potentialInsertionShiftedHash = (keyHash >> (HASH_BIT_SHIFT_AMOUNT_PER_LEVEL * (currDepth + 1)));
				const std::size_t newPotentialInsertionIndex = (potentialInsertionShiftedHash & ARRAY_NODE_HASH_BITMASK);

				const bool doesIndexCollisionExist = (newHeadNodeIndex == newPotentialInsertionIndex);

				if (!doesIndexCollisionExist) [[likely]]
					replacementArrayNodePtr->ChildNodeArr[newPotentialInsertionIndex].Store(potentialInsertionNodePtr, std::memory_order::relaxed);

				const bool wasCompareExchangeSuccessful = currAtomicNodePtr->CompareExchangeStrong(currHeadNodePtr, replacementArrayNodePtr, std::memory_order::release, std::memory_order::acquire);

				// If we were successful and we added potentialInsertionNodePtr to the array node, then we
				// can exit early.
				if (wasCompareExchangeSuccessful && !doesIndexCollisionExist)
					return true;

				// If we failed and the new value of currHeadNodePtr != nullptr AND is an array node - or 
				// if we succeeded, but we did not add potentialInsertionNodePtr to the array node - then 
				// it must now be an array node, so we advance to the next level.
				if (wasCompareExchangeSuccessful || (currHeadNodePtr != nullptr && currHeadNodePtr->Type == NodeType::ARRAY_NODE))
					++currDepth;

				// If we failed and the new value of currHeadNodePtr == nullptr, then we might be able to
				// replace it again. To do this, we move to the next loop iteration at the same depth level.
			}
			else
			{
				// If this is an array node, then just move to the next level.
				++currDepth;
			}
		}
	}

	template <typename Key, typename Value, typename LockType, typename Hasher>
	template <Brawler::Function<void, Value&> Callback>
	bool ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::AccessData(const Key& key, const Callback& callback)
	{
		const Brawler::SharedPtr<MapNode> relevantNodePtr{ TryFindExistingNodeForKey(key).Lock() };

		if (relevantNodePtr == nullptr) [[unlikely]]
			return false;

		relevantNodePtr->Data.AccessData(callback);
		return true;
	}

	template <typename Key, typename Value, typename LockType, typename Hasher>
	template <Brawler::Function<void, Value> Callback>
	bool ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::AccessData(const Key& key, const Callback& callback) const
	{
		const Brawler::SharedPtr<MapNode> relevantNodePtr{ TryFindExistingNodeForKey(key).Lock() };

		if (relevantNodePtr == nullptr) [[unlikely]]
			return false;

		relevantNodePtr->Data.AccessData(callback);
		return true;
	}

	template <typename Key, typename Value, typename LockType, typename Hasher>
	template <Brawler::Function<void, const Value&> Callback>
	bool ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::AccessData(const Key& key, const Callback& callback) const
	{
		const Brawler::SharedPtr<MapNode> relevantNodePtr{ TryFindExistingNodeForKey(key).Lock() };

		if (relevantNodePtr == nullptr) [[unlikely]]
			return false;

		relevantNodePtr->Data.AccessData(callback);
		return true;
	}

	template <typename Key, typename Value, typename LockType, typename Hasher>
	bool ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::Contains(const Key& key) const
	{
		const Brawler::SharedPtr<MapNode> relevantNodePtr{ TryFindExistingNodeForKey(key).Lock() };
		return (relevantNodePtr != nullptr);
	}

	template <typename Key, typename Value, typename LockType, typename Hasher>
	bool ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::Empty() const
	{
		// To check if the map is empty, we just need to see if none of the head nodes are
		// storing a nullptr. Since we are not actually reading any of the data pointed to
		// by the head node Brawler::SharedPtr instances, we can get away with only using
		// std::memory_order::relaxed.
		
		for (const auto& headNode : mHeadNodeArr)
		{
			const Brawler::SharedPtr<MapNode> headNodePtr{ headNode.Load(std::memory_order::relaxed) };

			if (headNodePtr != nullptr)
				return false;
		}

		return true;
	}

	template <typename Key, typename Value, typename LockType, typename Hasher>
	template <Brawler::Function<void, Value&> Callback>
	void ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::ForEach(const Callback& callback)
	{
		for (const auto& headNodePtr : mHeadNodeArr)
		{
			Brawler::SharedPtr<MapNode> currNodePtr{ headNodePtr.Load(std::memory_order::acquire) };

			if (currNodePtr != nullptr)
				currNodePtr->ForEachIMPL(callback);
		}
	}

	template <typename Key, typename Value, typename LockType, typename Hasher>
	template <Brawler::Function<void, const Value&> Callback>
	void ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::ForEach(const Callback& callback) const
	{
		for (const auto& headNodePtr : mHeadNodeArr)
		{
			Brawler::SharedPtr<MapNode> currNodePtr{ headNodePtr.Load(std::memory_order::acquire) };

			if (currNodePtr != nullptr)
				currNodePtr->ForEachIMPL(callback);
		}
	}

	template <typename Key, typename Value, typename LockType, typename Hasher>
	bool ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::Erase(const Key& key)
	{
		const std::size_t keyHash = Hasher{}(key);

		Brawler::AtomicSharedPtr<MapNode>* atomicNodePtr = nullptr;
		Brawler::SharedPtr<MapNode> currHeadNodePtr{};

		std::size_t currDepth = 0;

		while (true)
		{
			const std::size_t shiftedHash = (keyHash >> (HASH_BIT_SHIFT_AMOUNT_PER_LEVEL * currDepth));

			if (currDepth == 0)
				atomicNodePtr = &(mHeadNodeArr[shiftedHash & BASE_ARRAY_HASH_BITMASK]);
			else
				atomicNodePtr = &(currHeadNodePtr->ChildNodeArr[shiftedHash & ARRAY_NODE_HASH_BITMASK]);

			currHeadNodePtr = atomicNodePtr->Load(std::memory_order::acquire);

			if (currHeadNodePtr == nullptr)
				return false;

			if (currHeadNodePtr->Type == NodeType::ARRAY_NODE)
				++currDepth;
			else
			{
				if (currHeadNodePtr->HashValue != keyHash)
					return false;

				if (atomicNodePtr->CompareExchangeStrong(currHeadNodePtr, Brawler::SharedPtr<MapNode>{}, std::memory_order::relaxed, std::memory_order::acquire))
					return true;

				if (currHeadNodePtr != nullptr && currHeadNodePtr->Type == NodeType::ARRAY_NODE)
					++currDepth;
			}
		}
	}

	template <typename Key, typename Value, typename LockType, typename Hasher>
	void ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::Clear()
	{
		// Do an exchange operation on each head node pointer of the array. Since we are using
		// reference-counted SharedPtr instances, any threads which are currently accessing the
		// data will not access garbage data, so long as they do so with at least one strong or
		// weak reference to the relevant control block.
		//
		// In addition, since each node pointer manages the lifetime of the child nodes
		// immediately after it, by simply replacing the head node, we can delete the entire tree
		// at once in a chain.

		for (auto& atomicNodePtr : mHeadNodeArr)
			atomicNodePtr.Store(Brawler::SharedPtr<MapNode>{}, std::memory_order::relaxed);
	}

	template <typename Key, typename Value, typename LockType, typename Hasher>
	consteval bool ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::IsLockFree()
	{
		// The map itself should always be lock free so long as AtomicSharedPtr<T> and
		// std::atomic<bool> are both lock free, but we additionally provide the ability
		// to lock individual elements in the map. That way, no two threads can concurrently
		// access the same Value instance.
		//
		// Strictly speaking, then, we cannot say the ThreadSafeUnorderedMap object is
		// lock free if it is using locks to protect the data elements.
		
		return (Brawler::AtomicSharedPtr<MapNode>::IsLockFree() && ValueContainer<Value, LockType>::IsLockFree());
	}

	template <typename Key, typename Value, typename LockType, typename Hasher>
	Brawler::WeakPtr<typename ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::MapNode> ThreadSafeUnorderedMap<Key, Value, LockType, Hasher>::TryFindExistingNodeForKey(const Key& key) const
	{
		const std::size_t keyHash = Hasher{}(key);

		const Brawler::AtomicSharedPtr<MapNode>* atomicNodePtr = nullptr;
		Brawler::SharedPtr<MapNode> currHeadNodePtr{};

		std::size_t currDepth = 0;

		while (true)
		{
			const std::size_t shiftedHash = (keyHash >> (HASH_BIT_SHIFT_AMOUNT_PER_LEVEL * currDepth));

			if (currDepth == 0)
				atomicNodePtr = &(mHeadNodeArr[shiftedHash & BASE_ARRAY_HASH_BITMASK]);
			else
				atomicNodePtr = &(currHeadNodePtr->ChildNodeArr[shiftedHash & ARRAY_NODE_HASH_BITMASK]);

			currHeadNodePtr = atomicNodePtr->Load(std::memory_order::acquire);

			if (currHeadNodePtr == nullptr)
				return Brawler::WeakPtr<MapNode>{};

			if (currHeadNodePtr->Type == NodeType::DATA_NODE)
				return (currHeadNodePtr->HashValue == keyHash ? Brawler::WeakPtr<MapNode>{currHeadNodePtr} : Brawler::WeakPtr<MapNode>{});

			++currDepth;
		}
	}
}
module;
#include <vector>
#include <algorithm>
#include <functional>
#include <span>

export module Brawler.SortedVector;
import Brawler.Functional;

export namespace Brawler
{
	/// <summary>
	/// Much like std::set and std::unordered_set, SortedVector is a class which
	/// guarantees the existence of at most one instance of a given key element.
	/// 
	/// Unlike those data structures, SortedVector is implemented with a linear
	/// array, rather than linked lists. This makes it much more attractive in
	/// general than std::set, but perhaps not so compared to std::unordered_set.
	/// 
	/// Whereas SortedVector can perform a lookup in O(log n) worst-case time,
	/// std::unordered_set can do so in O(1) for the average case and O(n) for
	/// the worst-case time.
	/// 
	/// If you need the elements to be sorted, then SortedVector is generally
	/// a better choice than std::set. However, if they do not need to be
	/// sorted, then you should profile the difference between SortedVector
	/// and std::unordered_set.
	/// </summary>
	/// <typeparam name="T">
	/// - The type of the keys which are to be stored in the SortedVector instance.
	/// </typeparam>
	/// <typeparam name="ComparisonOp">
	/// - A type whose operator() overload determines the placement of elements 
	///   within the SortedVector instance. It has a function signature of 
	///   bool(const T&amp;, const T&amp;).
	///
	///   The operator() overload should return true if the element specified by
	///   the first parameter should appear before the element specified by the
	///   second parameter in the SortedVector.
	/// </typeparam>
	template <typename T, typename ComparisonOp = std::ranges::less>
	class SortedVector
	{
	public:
		SortedVector() = default;

		SortedVector(const SortedVector& rhs) = default;
		SortedVector& operator=(const SortedVector& rhs) = default;

		SortedVector(SortedVector&& rhs) noexcept = default;
		SortedVector& operator=(SortedVector&& rhs) noexcept = default;

		/// <summary>
		/// If the value specified by key does not already exist in the SortedVector
		/// instance, then this function will add it to its set of contained values;
		/// otherwise, this function does nothing.
		/// </summary>
		/// <param name="key">
		/// - The value which is to be inserted into the SortedVector instance, if
		///   it did not already exist within it.
		/// </param>
		template <typename U>
			requires std::is_same_v<std::decay_t<T>, std::decay_t<U>>
		void Insert(U&& key);

		void Remove(const T& key);

		/// <summary>
		/// Describes whether or not the value specified by key exists in this
		/// SortedVector instance.
		/// </summary>
		/// <param name="key">
		/// - The value whose existence within this SortedVector instance is to be
		///   verified.
		/// </param>
		/// <returns>
		/// The function returns true if this SortedVector instance contains the
		/// value specified by key and false otherwise.
		/// </returns>
		bool Contains(const T& key) const;

		/// <summary>
		/// Reserves enough space within the underlying array to hold newCapacity
		/// keys within this SortedVector instance.
		/// 
		/// If the number of elements is known ahead of time, then calling this
		/// function to specify the expected size of the SortedVector before inserting
		/// elements can drastically improve performance.
		/// </summary>
		/// <param name="newCapacity">
		/// - The requested capacity of the underlying array. After calling this
		///   function, the SortedVector will have allocated enough memory to hold
		///   at least newCapacity elements.
		/// </param>
		void Reserve(const std::size_t newCapacity);

		/// <summary>
		/// Retrieves the number of elements in the SortedVector instance.
		/// </summary>
		/// <returns>
		/// The function returns the number of elements in the SortedVector instance.
		/// </returns>
		std::size_t GetSize() const;

		bool Empty() const;

		template <Brawler::Function<void, const T&> Func>
		void ForEach(const Func& callback) const;

		std::span<const T> CreateSpan() const;

	private:
		std::vector<T> mDataArr;
	};
}

// -----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T, typename ComparisonOp>
	template <typename U>
		requires std::is_same_v<std::decay_t<T>, std::decay_t<U>>
	void SortedVector<T, ComparisonOp>::Insert(U&& key)
	{
		const auto itr = std::ranges::lower_bound(mDataArr, key, ComparisonOp{});

		// If the element does not already exist, then add it at the specified
		// position.
		if (itr == mDataArr.end() || *itr != key)
			mDataArr.insert(itr, std::forward<U>(key));
	}

	template <typename T, typename ComparisonOp>
	void SortedVector<T, ComparisonOp>::Remove(const T& key)
	{
		const auto itr = std::ranges::lower_bound(mDataArr, key, ComparisonOp{});

		if (itr != mDataArr.end() && *itr == key) [[likely]]
			mDataArr.erase(itr);
	}

	template <typename T, typename ComparisonOp>
	bool SortedVector<T, ComparisonOp>::Contains(const T& key) const
	{
		const auto itr = std::ranges::lower_bound(mDataArr, key, ComparisonOp{});
		return (itr != mDataArr.end() && *itr == key);
	}

	template <typename T, typename ComparisonOp>
	void SortedVector<T, ComparisonOp>::Reserve(const std::size_t newCapacity)
	{
		mDataArr.reserve(newCapacity);
	}

	template <typename T, typename ComparisonOp>
	std::size_t SortedVector<T, ComparisonOp>::GetSize() const
	{
		return mDataArr.size();
	}

	template <typename T, typename ComparisonOp>
	bool SortedVector<T, ComparisonOp>::Empty() const
	{
		return mDataArr.empty();
	}

	template <typename T, typename ComparisonOp>
	template <Brawler::Function<void, const T&> Func>
	void SortedVector<T, ComparisonOp>::ForEach(const Func& callback) const
	{
		for (const auto& element : mDataArr)
			callback(element);
	}

	template <typename T, typename ComparisonOp>
	std::span<const T> SortedVector<T, ComparisonOp>::CreateSpan() const
	{
		return std::span<const T>{ mDataArr };
	}
}
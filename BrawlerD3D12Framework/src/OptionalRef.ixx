module;
#include <optional>
#include <functional>
#include <cassert>

export module Brawler.OptionalRef;

namespace Brawler
{
	template <typename T>
	struct RefTypeDeducer
	{
		using RefType = std::decay_t<T>&;
	};

	template <typename T>
	struct RefTypeDeducer<const T>
	{
		using RefType = const std::decay_t<T>&;
	};
}

export namespace Brawler
{
	template <typename T>
	class OptionalRef
	{
	private:
		using ReferenceType = RefTypeDeducer<T>::RefType;
		using ReferenceWrapperType = std::reference_wrapper<std::remove_reference_t<ReferenceType>>;

	public:
		constexpr OptionalRef() = default;
		constexpr OptionalRef(RefTypeDeducer<T>::RefType val);

		constexpr OptionalRef(const OptionalRef& rhs) = default;
		constexpr OptionalRef& operator=(const OptionalRef& rhs) = default;

		constexpr OptionalRef(OptionalRef&& rhs) noexcept = default;
		constexpr OptionalRef& operator=(OptionalRef&& rhs) noexcept = default;

		constexpr bool HasValue() const;

		constexpr ReferenceType operator*() const;
		constexpr std::remove_reference_t<ReferenceType>* operator->() const;

	private:
		std::optional<ReferenceWrapperType> mRefWrapper;
	};
}

// --------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
	constexpr OptionalRef<T>::OptionalRef(typename RefTypeDeducer<T>::RefType val) :
		mRefWrapper(ReferenceWrapperType{ val })
	{}

	template <typename T>
	constexpr bool OptionalRef<T>::HasValue() const
	{
		return mRefWrapper.has_value();
	}

	template <typename T>
	constexpr OptionalRef<T>::ReferenceType OptionalRef<T>::operator*() const
	{
		assert(HasValue() && "ERROR: An attempt was made to de-reference an empty Brawler::OptionalRef instance!");
		return mRefWrapper->get();
	}

	template <typename T>
	constexpr std::remove_reference_t<typename OptionalRef<T>::ReferenceType>* OptionalRef<T>::operator->() const
	{
		assert(HasValue() && "ERROR: An attempt was made to de-reference an empty Brawler::OptionalRef instance!");
		return &(mRefWrapper->get());
	}
}
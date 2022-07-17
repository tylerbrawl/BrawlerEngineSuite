module;
#include <utility>

export module Brawler.ZSTDContext:WrappedZSTDContext;
import :UnderlyingZSTDContextTypes;
import :ZSTDContextQueue;

export namespace Brawler
{
	template <typename T>
		requires IsZSTDContextType<T>
	class WrappedZSTDContext
	{
	private:
		using ContextType = std::remove_pointer_t<typename T::pointer>;

	public:
		WrappedZSTDContext();

		~WrappedZSTDContext();

		WrappedZSTDContext(const WrappedZSTDContext& rhs) = delete;
		WrappedZSTDContext& operator=(const WrappedZSTDContext& rhs) = delete;

		WrappedZSTDContext(WrappedZSTDContext&& rhs) noexcept = default;
		WrappedZSTDContext& operator=(WrappedZSTDContext&& rhs) noexcept;

		ContextType* Get() const;

		ContextType& operator*() const;
		ContextType* operator->() const;

	private:
		void ReturnContext();

	private:
		T mUniqueContextPtr;
	};
}

// -------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
		requires IsZSTDContextType<T>
	WrappedZSTDContext<T>::WrappedZSTDContext() :
		mUniqueContextPtr(ZSTDContextQueue::GetInstance().GetZSTDContext<T>())
	{}

	template <typename T>
		requires IsZSTDContextType<T>
	WrappedZSTDContext<T>::~WrappedZSTDContext()
	{
		ReturnContext();
	}

	template <typename T>
		requires IsZSTDContextType<T>
	WrappedZSTDContext<T>& WrappedZSTDContext<T>::operator=(WrappedZSTDContext&& rhs) noexcept
	{
		ReturnContext();

		mUniqueContextPtr = std::move(rhs.mUniqueContextPtr);

		return *this;
	}

	template <typename T>
		requires IsZSTDContextType<T>
	WrappedZSTDContext<T>::ContextType* WrappedZSTDContext<T>::Get() const
	{
		return mUniqueContextPtr.get();
	}

	template <typename T>
		requires IsZSTDContextType<T>
	WrappedZSTDContext<T>::ContextType& WrappedZSTDContext<T>::operator*() const
	{
		return *mUniqueContextPtr;
	}

	template <typename T>
		requires IsZSTDContextType<T>
	WrappedZSTDContext<T>::ContextType* WrappedZSTDContext<T>::operator->() const
	{
		return mUniqueContextPtr.get();
	}

	template <typename T>
		requires IsZSTDContextType<T>
	void WrappedZSTDContext<T>::ReturnContext()
	{
		if (mUniqueContextPtr != nullptr)
			ZSTDContextQueue::GetInstance().ReturnZSTDContext(std::move(mUniqueContextPtr));
	}
}
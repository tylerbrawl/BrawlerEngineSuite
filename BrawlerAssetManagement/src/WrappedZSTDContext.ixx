module;
#include <memory>
#include <cassert>
#include <zstd.h>

export module Brawler.AssetManagement.ZSTDContext:WrappedZSTDContext;
import :UnderlyingZSTDContextTypes;
import :ZSTDContextQueue;

export namespace Brawler
{
	namespace AssetManagement
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
			WrappedZSTDContext& operator=(WrappedZSTDContext&& rhs) noexcept = default;

			ContextType* Get() const;

			ContextType& operator*() const;
			ContextType* operator->() const;

		private:
			T mUniqueContextPtr;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace AssetManagement
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
			ZSTDContextQueue::GetInstance().ReturnZSTDContext(std::move(mUniqueContextPtr));
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
	}
}
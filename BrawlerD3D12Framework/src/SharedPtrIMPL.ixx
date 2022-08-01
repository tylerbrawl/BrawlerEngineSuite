module;
#include <cassert>
#include <utility>
#include <atomic>

export module Brawler.SharedPtr:SharedPtrIMPL;
import :SharedPtrControlBlock;

namespace Brawler
{
	template <typename DataType>
	class AtomicSharedPtr;

	template <typename DataType>
	class SharedPtr;

	template <typename DataType>
	class WeakPtr;

	template <typename DataType, typename... Args>
		requires std::constructible_from<DataType, Args...>
	SharedPtr<DataType> MakeShared(Args&&... args);
}

export namespace Brawler
{
	template <typename DataType>
	class SharedPtr
	{
	private:
		template <typename OtherType>
		friend class AtomicSharedPtr;

		template <typename OtherType>
		friend class WeakPtr;

		template <typename OtherType, typename... Args>
			requires std::constructible_from<OtherType, Args...>
		friend SharedPtr<OtherType> MakeShared<OtherType, Args...>(Args&&... args);

	public:
		SharedPtr() = default;

	private:
		explicit SharedPtr(SharedPtrControlBlock<DataType>& controlBlock);

	public:
		~SharedPtr();

		SharedPtr(const SharedPtr& rhs);
		SharedPtr& operator=(const SharedPtr& rhs);

		SharedPtr(SharedPtr&& rhs) noexcept;
		SharedPtr& operator=(SharedPtr&& rhs) noexcept;

		DataType* Get() const;

		DataType& operator*() const;
		DataType* operator->() const;

	private:
		void ReleaseControl();

	private:
		SharedPtrControlBlock<DataType>* mControlBlockPtr;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename DataType>
	SharedPtr<DataType>::SharedPtr(const SharedPtr& rhs) :
		mControlBlockPtr(rhs.mControlBlockPtr)
	{
		if (mControlBlockPtr != nullptr) [[likely]]
			mControlBlockPtr->IncrementStrongCount();
	}

	template <typename DataType>
	SharedPtr<DataType>::SharedPtr(SharedPtrControlBlock<DataType>& controlBlock) :
		mControlBlockPtr(&controlBlock)
	{
		// Don't increment the strong count when a SharedPtr instance is manually assigned a
		// control block. The assumption is that the caller would already have done this.
	}

	template <typename DataType>
	SharedPtr<DataType>::~SharedPtr()
	{
		ReleaseControl();
	}

	template <typename DataType>
	SharedPtr<DataType>& SharedPtr<DataType>::operator=(const SharedPtr& rhs)
	{
		ReleaseControl();

		mControlBlockPtr = rhs.mControlBlockPtr;

		if (mControlBlockPtr != nullptr) [[likely]]
			mControlBlockPtr->IncrementStrongCount();

		return *this;
	}

	template <typename DataType>
	SharedPtr<DataType>::SharedPtr(SharedPtr&& rhs) noexcept :
		mControlBlockPtr(rhs.mControlBlockPtr)
	{
		rhs.mControlBlockPtr = nullptr;
	}

	template <typename DataType>
	SharedPtr<DataType>& SharedPtr<DataType>::operator=(SharedPtr<DataType>&& rhs) noexcept
	{
		ReleaseControl();

		mControlBlockPtr = rhs.mControlBlockPtr;
		rhs.mControlBlockPtr = nullptr;

		return *this;
	}

	template <typename DataType>
	DataType* SharedPtr<DataType>::Get() const
	{
		return (mControlBlockPtr == nullptr ? nullptr : std::addressof(mControlBlockPtr->GetManagedData()));
	}

	template <typename DataType>
	DataType& SharedPtr<DataType>::operator*() const
	{
		assert(mControlBlockPtr != nullptr && "ERROR: An attempt was made to dereference a Brawler::SharedPtr instance which was not managing any object!");
		return mControlBlockPtr->GetManagedData();
	}

	template <typename DataType>
	DataType* SharedPtr<DataType>::operator->() const
	{
		assert(mControlBlockPtr != nullptr && "ERROR: An attempt was made to dereference a Brawler::SharedPtr instance which was not managing any object!");
		return std::addressof(mControlBlockPtr->GetManagedData());
	}

	template <typename DataType>
	void SharedPtr<DataType>::ReleaseControl()
	{
		if (mControlBlockPtr != nullptr)
		{
			mControlBlockPtr->DecrementStrongCount();
			mControlBlockPtr = nullptr;
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	template <typename DataType>
	class WeakPtr
	{
	public:
		WeakPtr() = default;
		explicit WeakPtr(const SharedPtr<DataType>& sharedPtr);

		~WeakPtr();

		WeakPtr(const WeakPtr& rhs);
		WeakPtr& operator=(const WeakPtr& rhs);

		WeakPtr(WeakPtr&& rhs) noexcept;
		WeakPtr& operator=(WeakPtr&& rhs) noexcept;

		SharedPtr<DataType> Lock() const;

	private:
		void ReleaseControl();

	private:
		SharedPtrControlBlock<DataType>* mControlBlockPtr;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename DataType>
	WeakPtr<DataType>::WeakPtr(const SharedPtr<DataType>& sharedPtr) :
		mControlBlockPtr(sharedPtr.mControlBlockPtr)
	{
		if (mControlBlockPtr != nullptr) [[likely]]
			mControlBlockPtr->IncrementWeakCount();
	}

	template <typename DataType>
	WeakPtr<DataType>::~WeakPtr()
	{
		ReleaseControl();
	}

	template <typename DataType>
	WeakPtr<DataType>::WeakPtr(const WeakPtr& rhs) :
		mControlBlockPtr(rhs.mControlBlockPtr)
	{
		if (mControlBlockPtr != nullptr) [[likely]]
			mControlBlockPtr->IncrementWeakCount();
	}

	template <typename DataType>
	WeakPtr<DataType>& WeakPtr<DataType>::operator=(const WeakPtr& rhs)
	{
		ReleaseControl();

		mControlBlockPtr = rhs.mControlBlockPtr;

		if (mControlBlockPtr != nullptr) [[likely]]
			mControlBlockPtr->IncrementWeakCount();

		return *this;
	}

	template <typename DataType>
	WeakPtr<DataType>::WeakPtr(WeakPtr&& rhs) noexcept :
		mControlBlockPtr(rhs.mControlBlockPtr)
	{
		rhs.mControlBlockPtr = nullptr;
	}

	template <typename DataType>
	WeakPtr<DataType>& WeakPtr<DataType>::operator=(WeakPtr&& rhs) noexcept
	{
		ReleaseControl();

		mControlBlockPtr = rhs.mControlBlockPtr;
		rhs.mControlBlockPtr = nullptr;

		return *this;
	}

	template <typename DataType>
	SharedPtr<DataType> WeakPtr<DataType>::Lock() const
	{
		// Try to increment the strong count of the SharedPtrControlBlock instance. If
		// we succeeded, then we can safely create a SharedPtr from it; otherwise, the
		// last existing SharedPtr instance referring to it must have been destroyed,
		// and so we cannot create any additional ones.
		if (mControlBlockPtr == nullptr) [[unlikely]]
			return SharedPtr<DataType>{};

		const bool doOtherSharedPtrsExist = mControlBlockPtr->IncrementStrongCountIfNotZero();
		return (doOtherSharedPtrsExist ? SharedPtr<DataType>{ *mControlBlockPtr } : SharedPtr<DataType>{});
	}

	template <typename DataType>
	void WeakPtr<DataType>::ReleaseControl()
	{
		if (mControlBlockPtr != nullptr)
		{
			mControlBlockPtr->DecrementWeakCount();
			mControlBlockPtr = nullptr;
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	template <typename DataType, typename... Args>
		requires std::constructible_from<DataType, Args...>
	SharedPtr<DataType> MakeShared(Args&&... args)
	{
		SharedPtrControlBlock<DataType>* const controlBlockPtr = new SharedPtrControlBlock<DataType>{ std::forward<Args>(args)... };
		controlBlockPtr->IncrementStrongCount();

		return SharedPtr<DataType>{ *controlBlockPtr };
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	template <typename DataType>
	bool operator==(const SharedPtr<DataType>& lhs, const SharedPtr<DataType>& rhs)
	{
		return (lhs.Get() == rhs.Get());
	}

	template <typename DataType>
	bool operator==(const SharedPtr<DataType>& lhs, const std::nullptr_t rhs)
	{
		return (lhs.Get() == rhs);
	}

	template <typename DataType>
	bool operator==(const std::nullptr_t lhs, const SharedPtr<DataType>& rhs)
	{
		return (rhs == lhs);
	}
}
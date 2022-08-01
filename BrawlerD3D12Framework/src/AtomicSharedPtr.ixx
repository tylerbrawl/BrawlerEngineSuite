module;
#include <atomic>
#include <cassert>
#include <tuple>
#include "DxDef.h"

export module Brawler.SharedPtr:AtomicSharedPtr;
import :SharedPtrControlBlock;
import :SharedPtrIMPL;

/*
Brawler::AtomicSharedPtr

This is an atomic shared_ptr-like class which targets x86-64 hardware. It makes use of double-width
compare-exchange (DWCAS, where "CAS" stands for "compare-and-swap," an alternative name to compare-exchange)
operations in order to change both a control block pointer and a local reference count atomically. This results
in a lock-free data structure, assuming that heap allocations do not occur during use. (The structure itself
never dynamically allocates any memory, but it might call delete.)

The implementation is *NOT* portable. It makes use of the Windows intrinsics _InterlockedCompareExchange128() and
_ReadWriteBarrier(). The wait and notify functionality also makes use of WaitOnAddress() and WakeByAddressXXX().
As mentioned earlier, it also only works on x86-64 hardware. The class is *NOT* intended to be STL-compliant, although 
the API was designed in a similar manner to the std::atomic API.

The key to implementing lock-free atomic shared_ptr classes is understanding differential reference counting. (I
have also heard this idea be called "split reference counting," but I like the first term more because it describes
the idea better.) Essentially, the AtomicSharedPtr instance contains an AtomicControlBlock which has an additional
atomic counter, known here as the TransientWeakReferenceCount. The idea is that whenever a thread wants to read
the pointer value stored in the AtomicSharedPtr instance so that it can perform one of the reading atomic shared_ptr
operations, it increments the TransientWeakReferenceCount. Once it is finished using it within the atomic shared_ptr
instance, it attempts to decrement the TransientWeakReferenceCount.

When a thread does a DWCAS on the AtomicControlBlock and successfully changes the stored control block pointer,
it should increment the weak count of the associated SharedPtrControlBlock by the retrieved value of TransientWeakReferenceCount.
It should also set TransientWeakReferenceCount back to 0 atomically with the control block pointer change. This is
the essence of differential reference counting: TransientWeakReferenceCount determines how many threads are currently
accessing the control block pointer through the AtomicSharedPtr instance.

A thread which increments TransientWeakReferenceCount is expected to decrement TransientWeakReferenceCount, but *ONLY IF*
the control block pointer has not changed since it incremented TransientWeakReferenceCount. If we do a DWCAS in an
attempt to decrement TransientWeakReferenceCount and it fails because another thread has changed the control block
pointer, then this should be interpreted as another thread having seen that we were using the control block pointer.
In that case, we assume that thread is going to increment the weak counter (because we told it that we were using it)
and counteract the increment by subsequently decrementing the weak counter. It is important to note that we only have
to do this if the DWCAS fails because the control block pointer did not match; if the control block pointer never
changed*, then we can keep trying to decrement the TransientWeakReferenceCount.

One important implementation detail to mention is that if deletion occurs upon decrementing either the strong or weak
counter of a control block, then threads which switch out the stored control block pointer *MUST* increment the
weak count of the control block by TransientWeakReferenceCount *BEFORE* decrementing the strong count. This prevents
an early deletion of the control block.

*When we say "never changed," we also include the ABA case where the pointer appears to not have changed, but that's
only because several threads committed control block pointer changes until it finally goes back to the original value,
all before the thread changing TransientWeakReferenceCount could even know what was happening. This works because we
would still eventually be affecting the weak count of the control block pointer. Consider the following scenario:

- Thread A increments TransientWeakReferenceCount for ControlBlock 1.
- Thread B swaps out ControlBlock 1 for ControlBlock 2, incrementing the weak count of ControlBlock 1 by the value
  of TransientWeakReferenceCount associated with it. (This includes, but does not necessarily equal, the increment
  just done by Thread A.)
- Other threads continue to make other changes before Thread A wakes up. By the time Thread A wakes up, the stored
  control block pointer is again ControlBlock 1.
- Thread A attempts to decrement TransientWeakReferenceCount. Both success and failure have a defined status:
  - If successful, then eventually, some thread X is going to adjust the weak count of ControlBlock 1 by what is
    essentially -1.

  - If it failed and the stored control block pointer is still ControlBlock 1, then it can just try again.

  - If it failed and the stored control block pointer has changed, then Thread A assumes that some other thread
    has seen its increment of TransientWeakReferenceCount. (Thread B did, in fact, do this in the second step.) In 
	that case, it manually decrements the weak count of ControlBlock 1 and no longer attempts to change 
	TransientWeakReferenceCount.

Thus, we find that this method is immune to the ABA problem.

*NOTE*: Even though most member functions of Brawler::AtomicSharedPtr allow the specification of a std::memory_order
value for memory ordering semantics, the provided values are currently ignored. This is because there are no
memory order semantics for _InterlockedCompareExchange128() which allows for the elision of memory/compiler barriers
such as, e.g., InterlockedExchangeAcquire64(). However, the arguments are provided so as to maintain a consistent
API should we choose to alter the implementation to a form which does, in fact, support the memory ordering.
*/

#pragma warning(push)
#pragma warning(disable: 4100)  // Disable warnings for unreferenced parameters.

namespace Brawler
{
	template <typename DataType, typename... Args>
		requires std::constructible_from<DataType, Args...>
	AtomicSharedPtr<DataType> MakeAtomicShared(Args&&... args);
}

export namespace Brawler
{
	template <typename DataType>
	class alignas(16) AtomicSharedPtr
	{
	private:
		template <typename OtherType, typename... Args>
			requires std::constructible_from<OtherType, Args...>
		friend AtomicSharedPtr<OtherType> MakeAtomicShared<OtherType, Args...>(Args&&... args);

	private:
#pragma warning(push)
#pragma warning(disable: 4324)
		struct alignas(16) AtomicControlBlock
		{
			SharedPtrControlBlock<DataType>* SharedBlockPtr;
			std::uint64_t TransientWeakReferenceCount;

			bool CompareExchange(AtomicControlBlock& expected, const AtomicControlBlock& desired)
			{
				return InterlockedCompareExchange128(
					reinterpret_cast<volatile LONG64*>(this),
					desired.TransientWeakReferenceCount,
					reinterpret_cast<LONG64>(desired.SharedBlockPtr),
					reinterpret_cast<LONG64*>(&expected)
				);
			}
		};
#pragma warning(pop)

	public:
		AtomicSharedPtr() = default;
		explicit AtomicSharedPtr(const SharedPtr<DataType>& sharedPtr);

	private:
		explicit AtomicSharedPtr(SharedPtrControlBlock<DataType>& controlBlock);

	public:
		~AtomicSharedPtr();

		AtomicSharedPtr(const AtomicSharedPtr& rhs) = delete;
		AtomicSharedPtr& operator=(const AtomicSharedPtr& rhs) = delete;

		AtomicSharedPtr(AtomicSharedPtr&& rhs) noexcept;
		AtomicSharedPtr& operator=(AtomicSharedPtr&& rhs) noexcept;

		void Store(const Brawler::SharedPtr<DataType>& desired, const std::memory_order order = std::memory_order::seq_cst);
		Brawler::SharedPtr<DataType> Load(const std::memory_order order = std::memory_order::seq_cst) const;

		Brawler::SharedPtr<DataType> Exchange(const Brawler::SharedPtr<DataType>& desired, const std::memory_order order = std::memory_order::seq_cst);

		bool CompareExchangeWeak(Brawler::SharedPtr<DataType>& expected, const Brawler::SharedPtr<DataType>& desired, const std::memory_order success, const std::memory_order failure);
		bool CompareExchangeWeak(Brawler::SharedPtr<DataType>& expected, const Brawler::SharedPtr<DataType>& desired, const std::memory_order order = std::memory_order::seq_cst);

		bool CompareExchangeStrong(Brawler::SharedPtr<DataType>& expected, const Brawler::SharedPtr<DataType>& desired, const std::memory_order success, const std::memory_order failure);
		bool CompareExchangeStrong(Brawler::SharedPtr<DataType>& expected, const Brawler::SharedPtr<DataType>& desired, const std::memory_order order = std::memory_order::seq_cst);

		void Wait(const Brawler::SharedPtr<DataType>& old, const std::memory_order order = std::memory_order::seq_cst);

		void NotifyOne();
		void NotifyAll();

		static consteval bool IsLockFree();

	private:
		void ReleaseControl();

	private:
		/// <summary>
		/// We need to make mMasterControlBlock mutable in order to make functions like
		/// AtomicSharedPtr::Load() const member functions. I know that this is ugly as sin,
		/// but doing a const_cast in a const member function on a non-mutable member can
		/// lead to undefined behavior.
		/// </summary>
		mutable AtomicControlBlock mMasterControlBlock;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	__forceinline void EmitCompilerBarrier()
	{
		// This is marked as a deprecated function, but since the MSVC STL makes use of it,
		// I don't see it being removed any time soon. (The #pragma warning directives are
		// used to disable deprecation warnings. The reason _ReadWriteBarrier() is deprecated,
		// according to the MSDN, is that the std::atomic memory ordering semantics should
		// be used instead. Normally, I would 100% agree with them, but we cannot use std::atomic
		// here because we make use of double-width compare-exchange (DWCAS) operations.)
#pragma warning(push)
#pragma warning(disable: 4996)
		_ReadWriteBarrier();
#pragma warning(pop)
	}

	constexpr std::memory_order ConvertFailureCompareExchangeMemoryOrder(const std::memory_order order)
	{
		switch (order)
		{
		case std::memory_order::acq_rel:
			return std::memory_order::acquire;

		case std::memory_order::release:
			return std::memory_order::relaxed;

		default:
			return order;
		}
	}
}

namespace Brawler
{
	template <typename DataType>
	AtomicSharedPtr<DataType>::AtomicSharedPtr(const SharedPtr<DataType>& sharedPtr) :
		mMasterControlBlock(sharedPtr.mControlBlockPtr, 0)
	{
		EmitCompilerBarrier();
	}

	template <typename DataType>
	AtomicSharedPtr<DataType>::AtomicSharedPtr(SharedPtrControlBlock<DataType>& controlBlock) :
		mMasterControlBlock(&controlBlock, 0)
	{
		EmitCompilerBarrier();
	}

	template <typename DataType>
	AtomicSharedPtr<DataType>::~AtomicSharedPtr()
	{
		ReleaseControl();
	}

	template <typename DataType>
	AtomicSharedPtr<DataType>::AtomicSharedPtr(AtomicSharedPtr&& rhs) noexcept :
		mMasterControlBlock()
	{
		AtomicControlBlock expectedControlBlock{};
		const AtomicControlBlock desiredControlBlock{};

		while (!rhs.mMasterControlBlock.CompareExchange(expectedControlBlock, desiredControlBlock));

		mMasterControlBlock = expectedControlBlock;
		EmitCompilerBarrier();
	}

	template <typename DataType>
	AtomicSharedPtr<DataType>& AtomicSharedPtr<DataType>::operator=(AtomicSharedPtr&& rhs) noexcept
	{
		ReleaseControl();

		AtomicControlBlock expectedControlBlock{};
		const AtomicControlBlock desiredControlBlock{};

		while (!rhs.mMasterControlBlock.CompareExchange(expectedControlBlock, desiredControlBlock));

		mMasterControlBlock = expectedControlBlock;
		EmitCompilerBarrier();

		return *this;
	}

	template <typename DataType>
	void AtomicSharedPtr<DataType>::Store(const Brawler::SharedPtr<DataType>& desired, const std::memory_order order)
	{
		// For now, AtomicSharedPtr::Store() is implemented as an AtomicSharedPtr::Exchange() operation
		// which discards its return value. (The destructor of Brawler::SharedPtr will correctly decrement
		// the strong count of the control block whose lifetime is partially managed by it, so the state
		// remains sane.)
		//
		// To remain consistent with the std::atomic API, however, we still disallow the std::memory_order
		// values which std::atomic<T>::store() typically disallows. (According to the C++ specifications,
		// these wouldn't even provide any guarantees for our use case anyways, since we are discarding
		// the value of the exchange.)
		
		assert(order != std::memory_order::consume && order != std::memory_order::acquire && order != std::memory_order::acq_rel && "ERROR: An invalid std::memory_order value was specified in a call to AtomicSharedPtr::Store()!");

		std::ignore = Exchange(desired, order);
	}

	template <typename DataType>
	Brawler::SharedPtr<DataType> AtomicSharedPtr<DataType>::Load(const std::memory_order order) const
	{
		assert(order != std::memory_order::release && order != std::memory_order::acq_rel && "ERROR: An invalid std::memory_order value was specified in a call to AtomicSharedPtr::Load()!");

		// Increment the TransientWeakReferenceCount.
		AtomicControlBlock preIncrementControlBlock{};
		{
			while (true)
			{
				const AtomicControlBlock desiredControlBlock{
					.SharedBlockPtr = preIncrementControlBlock.SharedBlockPtr,
					.TransientWeakReferenceCount = (preIncrementControlBlock.TransientWeakReferenceCount + 1)
				};

				if (mMasterControlBlock.CompareExchange(preIncrementControlBlock, desiredControlBlock))
					break;
			}
		}

		// Increment the strong count of the SharedPtrControlBlock, since we will be handing it
		// off to the returned SharedPtr instance.
		AtomicControlBlock postIncrementControlBlock{
			.SharedBlockPtr = preIncrementControlBlock.SharedBlockPtr,
			.TransientWeakReferenceCount = (preIncrementControlBlock.TransientWeakReferenceCount + 1)
		};

		if (postIncrementControlBlock.SharedBlockPtr != nullptr)
			postIncrementControlBlock.SharedBlockPtr->IncrementStrongCount();

		// Decrement the TransientWeakReferenceCount.
		{
			while (true)
			{
				const AtomicControlBlock desiredControlBlock{
					.SharedBlockPtr = postIncrementControlBlock.SharedBlockPtr,
					.TransientWeakReferenceCount = (postIncrementControlBlock.TransientWeakReferenceCount - 1)
				};

				if (mMasterControlBlock.CompareExchange(postIncrementControlBlock, desiredControlBlock))
					break;

				// If we failed the compare-exchange and the pointer has changed, then we need
				// to revert our strong count change and make a new adjustment for the next
				// pointer. However, we don't want to loop again in this case, because we do not
				// want to modify the TransientWeakReferenceCount for an unrelated control block.
				//
				// Is this okay? As it turns out, we can do this because this is just a load
				// operation.
				if (postIncrementControlBlock.SharedBlockPtr != desiredControlBlock.SharedBlockPtr)
				{
					if (desiredControlBlock.SharedBlockPtr != nullptr)
					{
						// Since we told mMasterControlBlock that we were holding a weak reference
						// to the previous control block by incrementing TransientWeakReferenceCount,
						// we need to account for the fact that another thread will proceed to
						// add this value to its weak count. We do this by decrementing the weak count.
						desiredControlBlock.SharedBlockPtr->DecrementWeakCount();

						desiredControlBlock.SharedBlockPtr->DecrementStrongCount();
					}

					// Increment the strong count of postIncrementControlBlock, since we are going
					// to be assigning it to a Brawler::SharedPtr instance.
					if (postIncrementControlBlock.SharedBlockPtr != nullptr)
						postIncrementControlBlock.SharedBlockPtr->IncrementStrongCount();

					break;
				}
			}
		}

		return (postIncrementControlBlock.SharedBlockPtr != nullptr ? Brawler::SharedPtr<DataType>{ *(postIncrementControlBlock.SharedBlockPtr) } : Brawler::SharedPtr<DataType>{});
	}

	template <typename DataType>
	Brawler::SharedPtr<DataType> AtomicSharedPtr<DataType>::Exchange(const Brawler::SharedPtr<DataType>& desired, const std::memory_order order)
	{
		// Increment the strong count of desired.mControlBlockPtr, since we are eventually
		// going to be storing it within the AtomicSharedPtr instance.
		if (desired.mControlBlockPtr != nullptr)
			desired.mControlBlockPtr->IncrementStrongCount();

		// Exchange mMasterControlBlock with a new AtomicControlBlock describing the new
		// object being pointed to. The TransientWeakReferenceCount field is set to 0
		// because that field describes how many other threads currently are using
		// the control block from this AtomicSharedPtr instance.
		const AtomicControlBlock desiredControlBlock{
			.SharedBlockPtr = desired.mControlBlockPtr,
			.TransientWeakReferenceCount = 0
		};

		AtomicControlBlock prevMasterBlock{};
		while (!mMasterControlBlock.CompareExchange(prevMasterBlock, desiredControlBlock));

		const bool wasControlBlockStored = (prevMasterBlock.SharedBlockPtr != nullptr);

		if (wasControlBlockStored)
		{
			// We are not done yet. We need to adjust the weak count of the control block which was
			// previously found in mMasterControlBlock.

			// Increment the weak count to account for all of the threads which are using
			// this AtomicSharedPtr instance and have announced that they are using the control
			// block by incrementing TransientWeakReferenceCount.
			prevMasterBlock.SharedBlockPtr->IncrementWeakCount(static_cast<std::uint32_t>(prevMasterBlock.TransientWeakReferenceCount));

			// We do not need to alter the strong count of the control block. Essentially, we are 
			// transferring the share of the control block's ownership previously owned by the 
			// AtomicSharedPtr instance to the Brawler::SharedPtr instance which we are returning.
			//
			// Not only does this make sense semantically, but it also saves us from an atomic
			// operation.
		}

		return (wasControlBlockStored ? Brawler::SharedPtr<DataType>{ *(prevMasterBlock.SharedBlockPtr) } : Brawler::SharedPtr<DataType>{});
	}

	template <typename DataType>
	bool AtomicSharedPtr<DataType>::CompareExchangeWeak(Brawler::SharedPtr<DataType>& expected, const Brawler::SharedPtr<DataType>& desired, const std::memory_order success, const std::memory_order failure)
	{
		assert(failure != std::memory_order::release && failure != std::memory_order::acq_rel && "ERROR: An invalid std::memory_order value was specified for the failure case in a call to AtomicSharedPtr::CompareExchangeWeak()!");

		// Increment the TransientWeakReferenceCount.
		AtomicControlBlock preIncrementControlBlock{};
		{
			while (true)
			{
				const AtomicControlBlock desiredControlBlock{
					.SharedBlockPtr = preIncrementControlBlock.SharedBlockPtr,
					.TransientWeakReferenceCount = (preIncrementControlBlock.TransientWeakReferenceCount + 1)
				};

				if (mMasterControlBlock.CompareExchange(preIncrementControlBlock, desiredControlBlock))
					break;
			}
		}

		AtomicControlBlock postIncrementControlBlock{
			.SharedBlockPtr = preIncrementControlBlock.SharedBlockPtr,
			.TransientWeakReferenceCount = (preIncrementControlBlock.TransientWeakReferenceCount + 1)
		};

		// Now, it's time for some fun. We have incremented the TransientWeakReferenceCount, implying
		// that this thread is currently using the control block pointer contained within
		// mMasterControlBlock. Now that we can safely read the value, we check if the pointer
		// is the same as expected.
		while (true)
		{
			const bool didExpectedControlBlockMatch = (postIncrementControlBlock.SharedBlockPtr == expected.mControlBlockPtr);

			if (didExpectedControlBlockMatch)
			{
				// If we have a match, then we need to set the control block pointer to that
				// of desired.mControlBlockPtr. To do that, we first increment its strong count,
				// since we plan on storing it in the AtomicSharedPtr.
				if (desired.mControlBlockPtr != nullptr)
					desired.mControlBlockPtr->IncrementStrongCount();

				const AtomicControlBlock desiredControlBlock{
					.SharedBlockPtr = desired.mControlBlockPtr,
					.TransientWeakReferenceCount = 0
				};

				if (mMasterControlBlock.CompareExchange(postIncrementControlBlock, desiredControlBlock))
				{
					if (postIncrementControlBlock.SharedBlockPtr != nullptr)
					{
						// We succeeded in setting the control block pointer. We now adjust the weak count
						// of the previous control block to that of 
						// (postIncrementControlBlock.TransientWeakReferenceCount - 1).
						//
						// Why minus one, you might ask? The reason is that we had already incremented
						// TransientWeakReferenceCount with the assumption that another thread might end
						// up seeing that we were using the control block. However, since we succeeded
						// with the compare-exchange operation, we know that this couldn't have been
						// the case. By subtracting one, we account for all of the references held by
						// all other threads and exclude this one. This is legal because this thread will
						// no longer be holding a weak reference to the control block, since we are leaving
						// the function in the next, like, two statements.
						postIncrementControlBlock.SharedBlockPtr->IncrementWeakCount(static_cast<std::uint32_t>(postIncrementControlBlock.TransientWeakReferenceCount - 1));

						// We also need to adjust the strong count to account for the fact that this
						// AtomicSharedPtr instance is no longer sharing ownership with the control block.
						// To prevent early deletion, decrementing the strong count *MUST* be done after
						// incrementing the weak count.
						postIncrementControlBlock.SharedBlockPtr->DecrementStrongCount();
					}
					
					// We are finished, and the meta-compare-exchange passed!
					return true;
				}
				else
				{
					// Decrement the strong count of desired.mControlBlockPtr, since we ended up not being
					// able to store it in this AtomicSharedPtr instance.
					if (desired.mControlBlockPtr != nullptr)
						desired.mControlBlockPtr->DecrementStrongCount();
					
					if (postIncrementControlBlock.SharedBlockPtr != expected.mControlBlockPtr)
					{
						if (expected.mControlBlockPtr != nullptr)
						{
							// If the control block pointer changed during this operation, then we need to
							// decrement the weak count of expected.mControlBlockPtr because we previously
							// adjusted TransientWeakReferenceCount for it.
							expected.mControlBlockPtr->DecrementWeakCount();
						}
						
						// Set the control block of expected to whatever the previous compare-exchange
						// operation returned. We increment the strong count because we are assigning
						// the control block to a new Brawler::SharedPtr instance.
						const bool isNewControlBlockValid = (postIncrementControlBlock.SharedBlockPtr != nullptr);

						if (isNewControlBlockValid)
							postIncrementControlBlock.SharedBlockPtr->IncrementStrongCount();

						expected = (isNewControlBlockValid ? Brawler::SharedPtr<DataType>{ *(postIncrementControlBlock.SharedBlockPtr) } : Brawler::SharedPtr<DataType>{});
						return false;
					}

					// Keep going if the operation failed only because the TransientWeakReferenceCount was incorrect.
				}
			}
			else
			{
				// If we do not have a match, then we try to again decrement the TransientWeakReferenceCount.
				const AtomicControlBlock desiredControlBlock{
					.SharedBlockPtr = postIncrementControlBlock.SharedBlockPtr,
					.TransientWeakReferenceCount = (postIncrementControlBlock.TransientWeakReferenceCount - 1)
				};

				if (mMasterControlBlock.CompareExchange(postIncrementControlBlock, desiredControlBlock))
				{
					// If we succeeded, then we don't need to worry about adjusting any weak counts,
					// since we were able to change TransientWeakReferenceCount before any other
					// thread could see otherwise.

					// Increment the strong count of postIncrementControlBlock.SharedBlockPtr
					// because we are going to be assigning it to a Brawler::SharedPtr instance.
					const bool isNewControlBlockValid = (postIncrementControlBlock.SharedBlockPtr != nullptr);

					if (isNewControlBlockValid)
						postIncrementControlBlock.SharedBlockPtr->IncrementStrongCount();

					expected = (isNewControlBlockValid ? Brawler::SharedPtr<DataType>{ *(postIncrementControlBlock.SharedBlockPtr) } : Brawler::SharedPtr<DataType>{});
					return false;
				}
				else
				{
					if (postIncrementControlBlock.SharedBlockPtr != desiredControlBlock.SharedBlockPtr)
					{
						// If the control block pointer stored in mMasterControlBlock changed, then we ought
						// to decrement the weak count of desiredControlBlock.SharedBlockPtr, since some
						// other thread saw our adjustment to TransientWeakReferenceCount.
						if (desiredControlBlock.SharedBlockPtr != nullptr)
							desiredControlBlock.SharedBlockPtr->DecrementWeakCount();

						// Is it safe to proceed? We don't want to continue in this loop because we would
						// be adjusting TransientWeakReferenceCount for an entirely unrelated control block.
						//
						// The best thing to do, then, would be to exit. If 
						// postIncrementControlBlock.SharedBlockPtr != expected.mControlBlockPtr, then we
						// can definitively say that the meta-compare-exchange failed, and we can set the
						// control block pointer of expected to what we just got.
						//
						// What if they are equal, though? If we are going to exit the loop, then we don't
						// want to try storing desired.mControlBlockPtr into mMasterControlBlock. We can't
						// return true, either, because we were not able to set the value stored. What do
						// we return then?
						//
						// The answer is that we return false, just as if 
						// postIncrementControlBlock.SharedBlockPtr were not equal to expected.mControlBlockPtr.
						// The only difference is that we do not set the value of expected. To the user,
						// this behavior is equivalent to that of a spurious failure which occurred during
						// a call to std::atomic<T>::compare_exchange_weak(), so we can get away with it.
						if (postIncrementControlBlock.SharedBlockPtr != expected.mControlBlockPtr)
						{
							const bool isNewControlBlockValid = (postIncrementControlBlock.SharedBlockPtr != nullptr);

							if (isNewControlBlockValid)
								postIncrementControlBlock.SharedBlockPtr->IncrementStrongCount();

							expected = (isNewControlBlockValid ? Brawler::SharedPtr<DataType>{ *(postIncrementControlBlock.SharedBlockPtr) } : Brawler::SharedPtr<DataType>{});
						}

						return false;
					}

					// Keep going if the operation failed only because the TransientWeakReferenceCount was incorrect.
				}
			}
		}
	}

	template <typename DataType>
	bool AtomicSharedPtr<DataType>::CompareExchangeWeak(Brawler::SharedPtr<DataType>& expected, const Brawler::SharedPtr<DataType>& desired, const std::memory_order order)
	{
		// If order is specified incorrectly for the failure case, then std::atomic<T>::compare_exchange_XXX will
		// silently replace it. We will do the same.
		const std::memory_order failureOrder = ConvertFailureCompareExchangeMemoryOrder(order);

		return CompareExchangeWeak(
			expected,
			desired,
			order,
			failureOrder
		);
	}

	template <typename DataType>
	bool AtomicSharedPtr<DataType>::CompareExchangeStrong(Brawler::SharedPtr<DataType>& expected, const Brawler::SharedPtr<DataType>& desired, const std::memory_order success, const std::memory_order failure)
	{
		// Keep going until one of the following happens:
		//
		//   1. AtomicSharedPtr::CompareExchangeWeak() returns true, indicating that the compare-exchange operation
		//      completed successfully.
		//
		//   2. expected.mControlBlockPtr has changed. If this is the case, then AtomicSharedPtr::CompareExchangeWeak()
		//      failed without a spurious failure.

		assert(failure != std::memory_order::release && failure != std::memory_order::acq_rel && "ERROR: An invalid std::memory_order value was specified for the failure case in a call to AtomicSharedPtr::CompareExchangeStrong()!");

		SharedPtrControlBlock<DataType>* const originalExpectedControlBlockPtr = expected.mControlBlockPtr;

		while (true)
		{
			const bool wasOperationSuccessful = CompareExchangeWeak(
				expected,
				desired,
				success,
				failure
			);
			
			if (wasOperationSuccessful)
				return true;

			if (expected.mControlBlockPtr != originalExpectedControlBlockPtr)
				return false;
		}
	}

	template <typename DataType>
	bool AtomicSharedPtr<DataType>::CompareExchangeStrong(Brawler::SharedPtr<DataType>& expected, const Brawler::SharedPtr<DataType>& desired, const std::memory_order order)
	{
		const std::memory_order failureOrder = ConvertFailureCompareExchangeMemoryOrder(order);

		return CompareExchangeStrong(
			expected,
			desired,
			order,
			failureOrder
		);
	}

	template <typename DataType>
	void AtomicSharedPtr<DataType>::Wait(const Brawler::SharedPtr<DataType>& old, const std::memory_order order)
	{
		assert(order != std::memory_order::release && order != std::memory_order::acq_rel && "ERROR: An invalid std::memory_order value was specified in a call to AtomicSharedPtr::Wait()!");

		SharedPtrControlBlock<DataType>* oldControlBlockPtr = old.mControlBlockPtr;

		while (Load(order).mControlBlockPtr == oldControlBlockPtr)
		{
			// NOTE: Since WaitOnAddress() takes a volatile VOID* for its address to wait on, rather
			// than a const volatile VOID*, and since the MSDN does not explicitly state that the
			// value pointed to by the parameter is not modified by the function, we err on the side
			// of caution and refrain from making AtomicSharedPtr::Wait() a const member function.
			//
			// This is in contrast to std::atomic<T>::wait(), which typically is a const member
			// function.
			
			WaitOnAddress(
				&(mMasterControlBlock.SharedBlockPtr),
				&oldControlBlockPtr,
				sizeof(oldControlBlockPtr),
				INFINITE
			);
		}
	}

	template <typename DataType>
	void AtomicSharedPtr<DataType>::NotifyOne()
	{
		WakeByAddressSingle(&(mMasterControlBlock.SharedBlockPtr));
	}

	template <typename DataType>
	void AtomicSharedPtr<DataType>::NotifyAll()
	{
		WakeByAddressAll(&(mMasterControlBlock.SharedBlockPtr));
	}

	template <typename DataType>
	consteval bool AtomicSharedPtr<DataType>::IsLockFree()
	{
		return true;
	}

	template <typename DataType>
	void AtomicSharedPtr<DataType>::ReleaseControl()
	{
		AtomicControlBlock finalControlBlock{};
		const AtomicControlBlock desiredControlBlock{};

		while (!mMasterControlBlock.CompareExchange(finalControlBlock, desiredControlBlock));

		if (finalControlBlock.SharedBlockPtr != nullptr)
		{
			// Adjust the weak count based on the TransientWeakReferenceCount. We *MUST* do this before
			// decrementing the strong count in order to prevent early deletion.
			finalControlBlock.SharedBlockPtr->IncrementWeakCount(static_cast<std::uint32_t>(finalControlBlock.TransientWeakReferenceCount));

			// Decrement the strong count, now that the AtomicSharedPtr instance no longer
			// shared ownership of the control block.
			finalControlBlock.SharedBlockPtr->DecrementStrongCount();
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	template <typename DataType, typename... Args>
		requires std::constructible_from<DataType, Args...>
	AtomicSharedPtr<DataType> MakeAtomicShared(Args&&... args)
	{
		SharedPtrControlBlock<DataType>* const controlBlockPtr = new SharedPtrControlBlock<DataType>{ std::forward<Args>(args)... };
		controlBlockPtr->IncrementStrongCount();

		return AtomicSharedPtr<DataType>{ *controlBlockPtr };
	}
}

#pragma warning(pop)
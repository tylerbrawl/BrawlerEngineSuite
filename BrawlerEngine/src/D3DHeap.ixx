module;
#include <atomic>
#include <queue>
#include "DxDef.h"

export module Brawler.D3DHeap;
import Util.Math;
import Brawler.ResourceCreationInfo;
import Brawler.D3DHeapBuddyAllocator;

export namespace Brawler
{
	class D3DHeapPool;
}

export namespace Brawler
{
	enum class D3DHeapState
	{
		UNINITIALIZED,
		RESIDENT,
		EVICTED
	};

	class D3DHeap
	{
	private:
		struct RelevanceHistory
		{
		private:
			std::queue<std::uint32_t> HistoryQueue;
			std::uint32_t HistoryRelevanceCounter;

		public:
			RelevanceHistory();

			RelevanceHistory(const RelevanceHistory& rhs) = delete;
			RelevanceHistory& operator=(const RelevanceHistory& rhs) = delete;

			RelevanceHistory(RelevanceHistory&& rhs) noexcept = default;
			RelevanceHistory& operator=(RelevanceHistory&& rhs) noexcept = default;

			void UpdateHistory(const std::uint32_t newRelevanceCount);
			std::uint32_t GetHistoryCount() const;

			/// <summary>
			/// This function validates that the history of a heap has been tracked long
			/// enough to know if it can be considered for eviction or not.
			/// </summary>
			/// <returns>
			/// The function returns true if the history count should be used to consider
			/// if the owning ID3D12Heap should be evicted and false otherwise.
			/// </returns>
			bool IsHistoryFullyPrepared() const;
		};

	public:
		D3DHeap(D3DHeapPool& owningPool, const Brawler::AllowedD3DResourceType allowedType);

		D3DHeap(const D3DHeap& rhs) = delete;
		D3DHeap& operator=(const D3DHeap& rhs) = delete;

		D3DHeap(D3DHeap&& rhs) noexcept = default;
		D3DHeap& operator=(D3DHeap&& rhs) noexcept = default;

		/// <summary>
		/// Attempts to create the underlying ID3D12Heap for this D3DHeap instance using
		/// the specified D3D12_HEAP_DESC.
		/// </summary>
		/// <param name="heapDesc">
		/// - The D3D12_HEAP_DESC which describes the ID3D12Heap which will be created.
		/// Keep in mind that all heaps must be compliant with the Heap Tier 1 rules,
		/// even if we are dealing with a Heap Tier 2 device.
		/// </param>
		/// <returns>
		/// The function returns an HRESULT as follows:
		/// 
		///   - S_OK: The ID3D12Heap was created successfully.
		///   - E_OUTOFMEMORY: There is not enough video memory available to create the
		///     ID3D12Heap. If this happens, then we should try to either defragment other
		///     heaps or evict unused ones.
		/// 
		/// All other returned HRESULT values are undefined errors.
		/// </returns>
		HRESULT Initialize(const D3D12_HEAP_DESC& heapDesc);

		HRESULT AllocateResource(const Brawler::ResourceCreationInfo& creationInfo);

		bool WouldAllocationSucceed(const Brawler::ResourceCreationInfo& creationInfo);

		HRESULT MakeResident();
		HRESULT Evict();

		void PrepareForNextFrame();

		/// <summary>
		/// Notifies the D3DHeap that it was used, perhaps many times, during the current
		/// frame. 
		/// 
		/// (Under serious memory constraints, the system may have to evict heaps
		/// which are still in use. By tracking how many times a D3DHeap's resources
		/// were used during a single frame, we can set an appropriate residence priority.
		/// That way, if the system really needs to evict a heap which is still in use, it
		/// can prefer to evict the one which was used least frequently.)
		/// </summary>
		void IncrementCurrentRelevanceCount();

		D3DHeapType GetHeapType() const;
		const D3DHeapInfo& GetHeapInfo() const;
		std::uint64_t GetHeapSize() const;

	private:
		// This is a counter which keeps track of the number of times a resource stored
		// within this heap was specified as a resource dependency during a 
		// RenderJobChain/frame. 
		//
		// During the following Update(), the counter is pushed to the end of the queue
		// contained in mHistory. This queue keeps track of the number of times this
		// heap's resources were needed, up to a pre-defined amount. If, after a number
		// of frames, this tracked amount becomes zero, then we can assume that the heap
		// can be evicted.
		std::atomic<std::uint32_t> mRelevanceCounter;

		// This is the main structure responsible for tracking the usage of the ID3D12Heap
		// during a frame.
		RelevanceHistory mHistory;

		Microsoft::WRL::ComPtr<Brawler::D3D12Heap> mHeap;
		D3DHeapBuddyAllocator mAllocator;
		D3DHeapType mType;
		D3DHeapPool* const mOwningPool;
		D3DHeapState mState;

		const D3DHeapInfo mHeapInfo;
	};
}
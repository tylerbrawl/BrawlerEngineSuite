module;
#include <atomic>
#include "DxDef.h"

export module Brawler.D3D12.I_PageableGPUObject;

export namespace Brawler
{
	namespace D3D12
	{
		class I_PageableGPUObject
		{
		protected:
			I_PageableGPUObject();

		public:
			virtual ~I_PageableGPUObject();

			I_PageableGPUObject(const I_PageableGPUObject& rhs) = delete;
			I_PageableGPUObject& operator=(const I_PageableGPUObject& rhs) = delete;

			I_PageableGPUObject(I_PageableGPUObject&& rhs) noexcept = default;
			I_PageableGPUObject& operator=(I_PageableGPUObject&& rhs) noexcept = default;

			void SetResidencyStatus(const bool isResident);
			bool IsResident() const;

			/// <summary>
			/// In memory-constrained scenarios, the GPUResidencyManager will call this function
			/// on I_PageableGPUObject instances to check if they need to be made resident.
			/// 
			/// Derived classes should implement this function to return true unless they can
			/// guarantee that evicting the object from GPU memory would not adversely affect the
			/// runtime; in that case, they should return false. Note, however, that returning false
			/// does not guarantee that an I_PageableGPUObject will actually be evicted.
			/// 
			/// *NOTE*: The value returned by this function should not change based on the residency
			/// status of the I_PageableGPUObject.
			/// </summary>
			/// <returns>
			/// Derived classes should implement this function to return true unless they can
			/// guarantee that evicting the object from GPU memory would not adversely affect the
			/// runtime; in that case, they should return false.
			/// 
			/// *NOTE*: The value returned by this function should not change based on the residency
			/// status of the I_PageableGPUObject.
			/// </returns>
			virtual bool NeedsResidency() const = 0;

			/// <summary>
			/// The usage metric should be a value in the range [0.0f, 1.0f] which describes how often
			/// the I_PageableGPUObject has been used in the last ~30 frames. Higher values indicate
			/// an object has seen more use by the GPU. This value is used by the GPUResidencyManager
			/// to prefer evicting resources which are used least often.
			/// 
			/// Derived classes should implement this function to return the appropriate usage metric.
			/// </summary>
			/// <returns>
			/// Derived classes should implement this function to return the appropriate usage metric.
			/// See the description for more details.
			/// </returns>
			virtual float GetUsageMetric() const = 0;

			/// <summary>
			/// After determining the individual usage metrics of the I_PageableGPUObjects, the
			/// GPUResidencyManager then sorts by memory consumption to determine which objects
			/// to evict.
			/// 
			/// Derived classes should implement this function to return the size, in bytes, of
			/// the object in GPU memory. (This value should not change based on residency status.)
			/// </summary>
			/// <returns>
			/// Derived classes should implement this function to return the size, in bytes, of
			/// the object in GPU memory. (This value should not change based on residency status.)
			/// </returns>
			virtual std::size_t GetGPUMemorySize() const = 0;

			/// <summary>
			/// The ID3D12Device::MakeResident() and ID3D12Device::Evict() functions take
			/// ID3D12Pageable* values to refer to the objects which are to be paged in and out of
			/// the GPU memory.
			/// 
			/// Derived classes should implement this function to return a reference to the relevant
			/// ID3D12Pageable object which will be paged in and out.
			/// </summary>
			/// <returns>
			/// Derived classes should implement this function to return a reference to the relevant
			/// ID3D12Pageable object which will be paged in and out.
			/// </returns>
			virtual ID3D12Pageable& GetD3D12PageableObject() const = 0;

			/// <summary>
			/// Under extreme memory pressure (or when the device is incapable of freeing residency
			/// budget only through eviction), it becomes necessary to actually delete resources to
			/// free up residency, rather than merely evicting them.
			/// 
			/// Derived classes should implement this function to return true if, at the time the
			/// function is called, it would be safe to delete the object from GPU memory (ignoring
			/// any potential read access violations on the CPU caused by doing so) and false otherwise.
			/// 
			/// It is important to note that this is *NOT* the same as eviction! If an object gets
			/// evicted, then its contents persist to the disk. A deleted object's contents are
			/// lost forever upon deletion!
			/// 
			/// Also, returning true does not guarantee that the object will actually be deleted.
			/// Even if the GPUResidencyManager does decide to delete the object, the actual deletion
			/// will be postponed until it is safe to do so on the CPU side.
			/// </summary>
			/// <returns>
			/// Derived classes should implement this function to return true if, at the time the
			/// function is called, it would be safe to delete the object from GPU memory (ignoring
			/// any potential read access violations on the CPU caused by doing so) and false otherwise.
			/// 
			/// Refer to the summary for more important remarks.
			/// </returns>
			virtual bool IsDeletionSafe() const = 0;

			virtual void DeleteD3D12PageableObject() = 0;

		private:
			bool mIsResident;
		};
	}
}
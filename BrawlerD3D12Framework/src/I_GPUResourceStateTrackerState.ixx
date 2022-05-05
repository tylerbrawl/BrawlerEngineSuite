module;

export module Brawler.D3D12.I_GPUResourceStateTrackerState;
export import Brawler.D3D12.BaseGPUResourceStateTrackerState;

export namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedType>
		class I_GPUResourceStateTrackerState : public BaseGPUResourceStateTrackerState
		{
		protected:
			I_GPUResourceStateTrackerState() = default;

		public:
			virtual ~I_GPUResourceStateTrackerState() = default;

			bool ProcessResourceStateZone(const ResourceStateZone& stateZone);
			void OnStateDecayBarrier();
		};
	}
}

// -----------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedType>
		bool I_GPUResourceStateTrackerState<DerivedType>::ProcessResourceStateZone(const ResourceStateZone& stateZone)
		{
			return static_cast<DerivedType*>(this)->ProcessResourceStateZone(stateZone);
		}

		template <typename DerivedType>
		void I_GPUResourceStateTrackerState<DerivedType>::OnStateDecayBarrier()
		{
			static_cast<DerivedType*>(this)->OnStateDecayBarrier();
		}
	}
}
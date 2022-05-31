module;
#include <variant>
#include "DxDef.h"

export module Brawler.D3D12.GPUResourceEvent;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		struct ResourceTransitionEvent
		{
			std::uint32_t SubResourceIndex;
			D3D12_RESOURCE_STATES BeforeState;
			D3D12_RESOURCE_STATES AfterState;
			D3D12_RESOURCE_BARRIER_FLAGS Flags;
		};

		struct ResourceInitializationEvent
		{};

		struct UAVBarrierEvent
		{};

		enum class GPUResourceEventID
		{
			RESOURCE_TRANSITION,
			RESOURCE_INITIALIZATION,
			UAV_BARRIER,

			COUNT_OR_ERROR
		};

		struct GPUResourceEvent
		{
			I_GPUResource* GPUResource;
			
			std::variant<ResourceTransitionEvent, ResourceInitializationEvent, UAVBarrierEvent> Event;
			GPUResourceEventID EventID;
		};
	}
}
module;
#include <cassert>
#include "DxDef.h"

export module Util.D3D12:General;

import Util.Math;
import Brawler.D3D12.GPUMemoryBudgetInfo;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.I_GPUResource;
import Util.General;
import Util.Engine;
import Brawler.D3D12.GPUDevice;

export namespace Util
{
	namespace D3D12
	{
		struct ResourceTransitionCheckInfo
		{
			Brawler::D3D12::GPUCommandQueueType QueueType;
			D3D12_RESOURCE_STATES BeforeState;
			D3D12_RESOURCE_STATES AfterState;
		};
	}
}

namespace Util
{
	namespace D3D12
	{
		// Okay... Who's the idiot who forgot to make PIX_COLOR constexpr in the PIX header?
		__forceinline constexpr std::int32_t CalculatePIXColor(const std::uint8_t r, const std::uint8_t g, const std::uint8_t b)
		{
			std::int32_t colorValue = 0xFF000000;

			colorValue |= (static_cast<std::uint32_t>(r) << 16);
			colorValue |= (static_cast<std::uint32_t>(g) << 8);
			colorValue |= static_cast<std::uint32_t>(b);

			return colorValue;
		}
	}
}

export namespace Util
{
	namespace D3D12
	{
		constexpr std::int32_t PIX_EVENT_COLOR_CPU_ONLY = CalculatePIXColor(0x00, 0x00, 0xFF);
		constexpr std::int32_t PIX_EVENT_COLOR_CPU_GPU = CalculatePIXColor(0x00, 0xFF, 0x00);
		
		/// <summary>
		/// This function can be used to check if a particular resource state is valid.
		/// </summary>
		/// <param name="resourceState">
		/// - The D3D12_RESOURCE_STATES value whose validity is to be checked.
		/// </param>
		/// <returns>
		/// The function returns true if, according to the MSDN, an ID3D12Resource can
		/// legally be in the state specified by resourceState and false otherwise.
		/// </returns>
		constexpr bool IsResourceStateValid(const D3D12_RESOURCE_STATES resourceState);

		constexpr bool IsValidReadState(const D3D12_RESOURCE_STATES resourceState);
		constexpr bool IsValidWriteState(const D3D12_RESOURCE_STATES resourceState);

		bool IsImplicitStateTransitionPossible(const Brawler::D3D12::I_GPUResource& resource, const D3D12_RESOURCE_STATES promotedState);

		constexpr bool CanQueuePerformResourceTransition(const Brawler::D3D12::GPUCommandQueueType queueType, const D3D12_RESOURCE_STATES involvedState);
		constexpr bool CanQueuePerformResourceTransition(const ResourceTransitionCheckInfo& transitionCheckInfo);

		__forceinline constexpr D3D12_RESOURCE_STATES GetAlwaysPromotableResourceStatesMask();

		Brawler::D3D12::GPUMemoryBudgetInfo GetGPUMemoryBudgetInfo();

		bool IsDebugLayerEnabled();
		consteval bool IsPIXRuntimeSupportEnabled();
	}
}

// ----------------------------------------------------------------------------------------------------------

namespace Util
{
	namespace D3D12
	{
		static constexpr D3D12_RESOURCE_STATES ALL_READ_STATES_MASK = (
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDEX_BUFFER |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_READ |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PREDICATION |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VIDEO_DECODE_READ |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ
			);

		static constexpr D3D12_RESOURCE_STATES ALL_WRITE_STATES_MASK = (
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_DEPTH_WRITE |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_STREAM_OUT |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE |
			D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE
			);

		constexpr bool IsResourceStateValid(const D3D12_RESOURCE_STATES resourceState)
		{
			return (IsValidReadState(resourceState) || IsValidWriteState(resourceState));
		}

		constexpr bool IsValidReadState(const D3D12_RESOURCE_STATES resourceState)
		{
			// A resource can be in any number of read states, but if it is in a read state,
			// then it cannot be in a write state. (NOTE: The underlying value of
			// D3D12_RESOURCE_STATE_COMMON is 0, so this is legal.)

			return ((resourceState & ALL_WRITE_STATES_MASK) == 0);
		}

		constexpr bool IsValidWriteState(const D3D12_RESOURCE_STATES resourceState)
		{
			if (resourceState == D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON)
				return true;

			// If a resource is in a write state, then it cannot be in any read states.
			if ((resourceState & ALL_READ_STATES_MASK) != 0)
				return false;

			// If it is not in a read state, then it can only be in one write state.
			return (Util::Math::CountOneBits(std::to_underlying(resourceState)) == 1);
		}

		bool IsImplicitStateTransitionPossible(const Brawler::D3D12::I_GPUResource& resource, const D3D12_RESOURCE_STATES promotedState)
		{
			if (!IsResourceStateValid(promotedState))
				return false;

			const Brawler::D3D12_RESOURCE_DESC& resourceDesc{ resource.GetResourceDescription() };
			const bool isImplicitTransitionAlwaysAllowed = (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION::D3D12_RESOURCE_DIMENSION_BUFFER || (resourceDesc.Flags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) != 0);

			return (isImplicitTransitionAlwaysAllowed || (promotedState & GetAlwaysPromotableResourceStatesMask()) == promotedState);
		}

		constexpr bool CanQueuePerformResourceTransition(const Brawler::D3D12::GPUCommandQueueType queueType, const D3D12_RESOURCE_STATES involvedState)
		{
			constexpr D3D12_RESOURCE_STATES ALLOWED_STATES_ON_COPY_QUEUE_MASK = (
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST |
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE
				);

			constexpr D3D12_RESOURCE_STATES ALLOWED_STATES_ON_COMPUTE_QUEUE_MASK = (
				ALLOWED_STATES_ON_COPY_QUEUE_MASK |
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
				);

			switch (queueType)
			{
			case Brawler::D3D12::GPUCommandQueueType::DIRECT:
				return true;

			case Brawler::D3D12::GPUCommandQueueType::COMPUTE:
				return ((involvedState & ALLOWED_STATES_ON_COMPUTE_QUEUE_MASK) == involvedState);

			case Brawler::D3D12::GPUCommandQueueType::COPY:
				return ((involvedState & ALLOWED_STATES_ON_COPY_QUEUE_MASK) == involvedState);

			default:
				assert(false);
				std::unreachable();

				return false;
			}
		}

		constexpr bool CanQueuePerformResourceTransition(const ResourceTransitionCheckInfo& transitionCheckInfo)
		{
			return (CanQueuePerformResourceTransition(transitionCheckInfo.QueueType, transitionCheckInfo.BeforeState) &&
				CanQueuePerformResourceTransition(transitionCheckInfo.QueueType, transitionCheckInfo.AfterState));
		}

		__forceinline constexpr D3D12_RESOURCE_STATES GetAlwaysPromotableResourceStatesMask()
		{
			return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST |
				D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE;
		}

		Brawler::D3D12::GPUMemoryBudgetInfo GetGPUMemoryBudgetInfo()
		{
			Brawler::D3D12::GPUMemoryBudgetInfo budgetInfo{};

			Util::General::CheckHRESULT(Util::Engine::GetDXGIAdapter().QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &(budgetInfo.DeviceLocalMemoryInfo)));
			Util::General::CheckHRESULT(Util::Engine::GetDXGIAdapter().QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP::DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &(budgetInfo.SystemMemoryInfo)));

			return budgetInfo;
		}

		bool IsDebugLayerEnabled()
		{
			// In Release builds, we know that the Debug Layer isn't going to be enabled, so we
			// can just skip the check entirely. This should allow for better optimization.
			if constexpr (!Util::General::IsDebugModeEnabled())
				return false;
			else
			{
				static const bool isDebugLayerEnabled = Util::Engine::GetGPUDevice().IsDebugLayerEnabled();
				return isDebugLayerEnabled;
			}
		}

		consteval bool IsPIXRuntimeSupportEnabled()
		{
			// The PIX_EVENTS_ARE_TURNED_ON macro is defined by PIX if it is able to enable PIX events.
			// This macro is not defined in Release builds, but it *IS* defined in Release with Debugging
			// builds.

			constexpr bool ENABLE_PIX_RUNTIME_SUPPORT = true;

#ifdef PIX_EVENTS_ARE_TURNED_ON
			return ENABLE_PIX_RUNTIME_SUPPORT;
#else
			return false;
#endif
		}
	}
}
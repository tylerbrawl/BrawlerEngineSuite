module;
#include <memory>
#include <vector>
#include <cassert>
#include <variant>
#include <unordered_map>
#include <ranges>
#include <array>
#include <span>
#include "DxDef.h"

export module Brawler.D3D12.GPUCommandListRecorder;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.TypeTraits;
import Brawler.D3D12.GPUCommandContexts;
import Util.Engine;
import Util.D3D12;
export import Brawler.D3D12.GPUCommandManager;
import Brawler.D3D12.I_RenderPass;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.AliasedGPUMemoryManager;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.D3D12.GPUResourceStateManagement;

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		class GPUCommandListRecorder
		{
		private:
			struct AliasingBarrier
			{
				const I_GPUResource* ResourceBefore;
				const I_GPUResource* ResourceAfter;
			};

		private:
			using AliasingBarrierMap = std::unordered_map<const I_RenderPass<QueueType>*, std::vector<AliasingBarrier>>;

		public:
			GPUCommandListRecorder() = default;

			GPUCommandListRecorder(const GPUCommandListRecorder& rhs) = delete;
			GPUCommandListRecorder& operator=(const GPUCommandListRecorder& rhs) = delete;

			GPUCommandListRecorder(GPUCommandListRecorder&& rhs) noexcept = default;
			GPUCommandListRecorder& operator=(GPUCommandListRecorder&& rhs) noexcept = default;

			void TrackResourceAliasing(AliasedGPUMemoryManager& aliasedMemoryManager);

			void AddRenderPass(std::unique_ptr<I_RenderPass<QueueType>>&& renderPass);
			void Reserve(const std::size_t numRenderPasses);

			/// <summary>
			/// Given a GPUResourceEventManager, this function will extract all of the GPUResourceEvents
			/// for all of the I_RenderPass instances which will be recorded on this
			/// GPUCommandListRecorder instance. The returned std::vector will contain the
			/// GPUResourceEvents which this GPUCommandListRecorder's queue is not capable of handling.
			/// 
			/// The returned events should be performed on an I_RenderPass which is both executed on a
			/// queue capable of performing it and is guaranteed to execute before *ANY* of the
			/// I_RenderPass instances recorded by this GPUCommandListRecorder instance on the GPU
			/// timeline.
			/// </summary>
			/// <param name="eventManager">
			/// - The GPUResourceEventManager from which the relevant GPUResourceEvents will be extracted.
			/// </param>
			/// <returns>
			/// For GPUCommandListRecorder instances which record commands for the DIRECT queue, the
			/// returned std::vector will always be empty. For all other GPUCommandListRecorder instances,
			/// the returned std::vector will contain the set of GPUResourceEvents which this
			/// GPUCommandListRecorder instance's queue is incapable of handling.
			/// </returns>
			std::vector<GPUResourceEvent> PrepareGPUResourceEvents(GPUResourceEventManager& eventManager);

			/// <summary>
			/// Describes whether or not this GPUCommandListRecorder instance has any commands to submit
			/// to the GPU. This occurs if it has at least one GPUResourceEvent or at least one
			/// I_RenderPass instance to record.
			/// 
			/// If this function returns false, then it is perfectly valid to not send the
			/// GPUCommandListRecorder instance's GPUCommandContext to a GPUCommandContextSubmissionPoint.
			/// </summary>
			/// <returns>
			/// The function returns true if this GPUCommandListRecorder instance has any commands to
			/// submit to the GPU and false otherwise.
			/// </returns>
			bool HasUsefulCommands() const;

			void RecordCommandList();
			std::unique_ptr<GPUCommandQueueContextType<QueueType>> ExtractGPUCommandContext();

		private:
			void RecordRenderPass(const I_RenderPass<QueueType>& renderPass);
			void RecordGPUResourceEventsForRenderPass(const I_RenderPass<QueueType>& renderPass);

		private:
			std::unique_ptr<GPUCommandQueueContextType<QueueType>> mContext;
			std::vector<std::unique_ptr<I_RenderPass<QueueType>>> mRenderPassArr;
			GPUResourceEventManager mResourceEventManager;
			AliasingBarrierMap mAliasingBarrierMap;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		bool CanQueueHandleGPUResourceEvent(const GPUResourceEvent& resourceEvent)
		{
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
			{
				// DIRECT queues can handle any type of GPUResourceEvent.
				return true;
			}

			else if constexpr (QueueType == GPUCommandQueueType::COMPUTE)
			{
				// The COMPUTE queue can handle UAV barrier events and some types of resource
				// transition events.
				bool isValidTransition = false;

				if (resourceEvent.EventID == GPUResourceEventID::RESOURCE_TRANSITION)
				{
					const ResourceTransitionEvent& transitionEvent{ std::get<ResourceTransitionEvent>(resourceEvent.Event) };
					isValidTransition = Util::D3D12::CanQueuePerformResourceTransition(Util::D3D12::ResourceTransitionCheckInfo{
						.QueueType = QueueType,
						.BeforeState = transitionEvent.BeforeState,
						.AfterState = transitionEvent.AfterState
					});
				}

				else if (resourceEvent.EventID == GPUResourceEventID::UAV_BARRIER)
					isValidTransition = true;

				return isValidTransition;
			}

			else
			{
				// The COPY queue can only handle a small subset of resource transition events
				// (even smaller than the COMPUTE queue can handle).
				if (resourceEvent.EventID != GPUResourceEventID::RESOURCE_TRANSITION)
					return false;

				const ResourceTransitionEvent& transitionEvent{ std::get<ResourceTransitionEvent>(resourceEvent.Event) };
				return Util::D3D12::CanQueuePerformResourceTransition(Util::D3D12::ResourceTransitionCheckInfo{
					.QueueType = QueueType,
					.BeforeState = transitionEvent.BeforeState,
					.AfterState = transitionEvent.AfterState
				});
			}
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		void GPUCommandListRecorder<QueueType>::TrackResourceAliasing(AliasedGPUMemoryManager& aliasedMemoryManager)
		{
			for (const auto& renderPass : mRenderPassArr)
			{
				for (const auto& resourceDependency : renderPass->GetResourceDependencies())
				{
					const I_GPUResource* const resourceBefore = aliasedMemoryManager.ActivateGPUResource(*(resourceDependency.ResourcePtr));

					if (resourceBefore != nullptr) [[unlikely]]
						mAliasingBarrierMap[renderPass.get()].push_back(AliasingBarrier{
							.ResourceBefore = resourceBefore,
							.ResourceAfter = resourceDependency.ResourcePtr
						});
				}
			}
		}

		template <GPUCommandQueueType QueueType>
		void GPUCommandListRecorder<QueueType>::AddRenderPass(std::unique_ptr<I_RenderPass<QueueType>>&& renderPass)
		{
			if (mContext == nullptr) [[unlikely]]
				mContext = Util::Engine::GetGPUCommandManager().GetGPUCommandContextVault().AcquireCommandContext<QueueType>();

			mRenderPassArr.push_back(std::move(renderPass));
		}

		template <GPUCommandQueueType QueueType>
		void GPUCommandListRecorder<QueueType>::Reserve(const std::size_t numRenderPasses)
		{
			mRenderPassArr.reserve(numRenderPasses);
		}

		template <GPUCommandQueueType QueueType>
		std::vector<GPUResourceEvent> GPUCommandListRecorder<QueueType>::PrepareGPUResourceEvents(GPUResourceEventManager& eventManager)
		{
			std::vector<GPUResourceEvent> impossibleEventArr{};

			for (const auto& renderPass : mRenderPassArr)
			{
				std::optional<GPUResourceEvent> renderPassEvent{};

				do
				{
					renderPassEvent = eventManager.ExtractGPUResourceEvent(*renderPass);

					if (renderPassEvent.has_value())
					{
						if (CanQueueHandleGPUResourceEvent<QueueType>(*renderPassEvent))
							mResourceEventManager.AddGPUResourceEvent(*renderPass, std::move(*renderPassEvent));
						else
							impossibleEventArr.push_back(std::move(*renderPassEvent));
					}
				} while (renderPassEvent.has_value());
			}

			return impossibleEventArr;
		}

		template <GPUCommandQueueType QueueType>
		bool GPUCommandListRecorder<QueueType>::HasUsefulCommands() const
		{
			return (mContext != nullptr && mContext->HasCommands());
		}

		template <GPUCommandQueueType QueueType>
		void GPUCommandListRecorder<QueueType>::RecordCommandList()
		{
			assert(mContext != nullptr && "ERROR: A GPUCommandListRecorder was never assigned any I_RenderPass instances to record into an ID3D12GraphicsCommandList!");
			assert(mContext->ReadyForUse() && "ERROR: A GPUCommandListRecorder was given a context which was not ready to have commands recorded into it!");

			mContext->ResetCommandList();

			for (const auto& renderPass : mRenderPassArr)
				RecordRenderPass(*renderPass);

			mContext->CloseCommandList();
		}

		template <GPUCommandQueueType QueueType>
		std::unique_ptr<GPUCommandQueueContextType<QueueType>> GPUCommandListRecorder<QueueType>::ExtractGPUCommandContext()
		{
			return std::move(mContext);
		}

		template <GPUCommandQueueType QueueType>
		void GPUCommandListRecorder<QueueType>::RecordRenderPass(const I_RenderPass<QueueType>& renderPass)
		{
			RecordGPUResourceEventsForRenderPass(renderPass);
			
			if (renderPass.RecordRenderPassCommands(*mContext)) [[likely]]
				mContext->MarkAsUseful();
		}

		template <GPUCommandQueueType QueueType>
		void GPUCommandListRecorder<QueueType>::RecordGPUResourceEventsForRenderPass(const I_RenderPass<QueueType>& renderPass)
		{
			// The idea is that we want to issue aliasing barriers before all other
			// types of barriers. As nice as it would be to issue all of these barriers
			// in one call to ID3D12GraphicsCommandList::ResourceBarrier(), special resource
			// initialization means that this is not always the case.
			//
			// Therefore, we create two batches of barriers. The first batch contains all
			// aliasing barriers, while the second batch contains all other barriers. If
			// we find that a resource whose barrier is in the second batch does not need
			// special resource initialization, then said barrier can safely be moved
			// into the first batch, after the aliasing barriers.
			//
			// You might benefit from knowing why this is safe. There are indeed a few cases
			// which one would normally need to be cautious of:
			//
			//   - Executing an aliasing barrier to alias out a resource after a begin split
			//     barrier was already issued for it. The MSDN does not explicitly state that
			//     this is illegal. However, even if doing this is allowed, it will never
			//     happen in the Brawler Engine. The reason is that GPUResourceEvents stop
			//     being generated after a resource's last use in an I_RenderPass. So, we
			//     will never see a begin split barrier with no matching end split barrier.

			std::vector<CD3DX12_RESOURCE_BARRIER> firstBarrierBatch{};
			std::vector<CD3DX12_RESOURCE_BARRIER> secondBarrierBatch{};

			// Algorithmically, it might make more sense to implement this with a
			// std::unordered_map for average constant look-up time. In practice, however,
			// since std::unordered_map is implemented with linked lists and we don't expect
			// this std::vector to be too large in most cases, it would probably perform
			// better to just do a linear search. That way, we can take advantage of the cache
			// friendliness of the glorious std::vector.
			std::vector<GPUResourceEvent> specialResourceInitializationEventArr{};

			// First, we perform all aliasing barriers.
			if (mAliasingBarrierMap.contains(&renderPass))
			{
				firstBarrierBatch.reserve(mAliasingBarrierMap.at(&renderPass).size());

				for (const auto& aliasingBarrier : mAliasingBarrierMap.at(&renderPass))
					firstBarrierBatch.push_back(CD3DX12_RESOURCE_BARRIER::Aliasing(&(aliasingBarrier.ResourceBefore->GetD3D12Resource()), &(aliasingBarrier.ResourceAfter->GetD3D12Resource())));
			}

			// Next, we transform any applicable GPUResourceEvents into D3D12 barriers and
			// check for special resource initialization.
			std::optional<GPUResourceEvent> resourceEvent{ mResourceEventManager.ExtractGPUResourceEvent(renderPass) };
			while (resourceEvent.has_value())
			{
				switch (resourceEvent->EventID)
				{
				case GPUResourceEventID::RESOURCE_TRANSITION:
				{
					const ResourceTransitionEvent& transitionEvent{ std::get<ResourceTransitionEvent>(resourceEvent->Event) };
					secondBarrierBatch.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
						&(resourceEvent->GPUResource->GetD3D12Resource()),
						transitionEvent.BeforeState,
						transitionEvent.AfterState,
						D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
						transitionEvent.Flags
					));

					break;
				}

				case GPUResourceEventID::RESOURCE_INITIALIZATION:
				{
					specialResourceInitializationEventArr.push_back(std::move(*resourceEvent));
					break;
				}

				case GPUResourceEventID::UAV_BARRIER:
				{
					secondBarrierBatch.push_back(CD3DX12_RESOURCE_BARRIER::UAV(&(resourceEvent->GPUResource->GetD3D12Resource())));
					break;
				}

				default:
					assert(false);
					__assume(false);
					break;
				}

				resourceEvent = mResourceEventManager.ExtractGPUResourceEvent(renderPass);
			}

			// The D3D12 API encourages us to batch as many resource barriers together as
			// possible. So, if we find a barrier in secondBarrierBatch whose corresponding
			// resource does not have a SpecialResourceInitialization event, then we move
			// it into firstBarrierBatch, after all of the aliasing barriers.
			//
			// However, since we are storing raw D3D12 barriers, we need to do the comparison
			// between ID3D12Resource* values, and not I_GPUResource* values.
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
			{
				for (auto itr = secondBarrierBatch.begin(); itr != secondBarrierBatch.end();)
				{
					const ID3D12Resource* d3d12ResourcePtr = nullptr;

					switch (itr->Type)
					{
					case D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
					{
						d3d12ResourcePtr = static_cast<D3D12_RESOURCE_BARRIER*>(&(*itr))->Transition.pResource;
						break;
					}

					case D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_UAV:
					{
						d3d12ResourcePtr = static_cast<D3D12_RESOURCE_BARRIER*>(&(*itr))->UAV.pResource;
						break;
					}

					default:
						assert(false);
						__assume(false);
						break;
					}

					auto findResult = std::ranges::find_if(specialResourceInitializationEventArr, [d3d12ResourcePtr] (const GPUResourceEvent& initializationEvent) { return (&(initializationEvent.GPUResource->GetD3D12Resource()) == d3d12ResourcePtr); });

					if (findResult == specialResourceInitializationEventArr.end()) [[likely]]
					{
						firstBarrierBatch.push_back(std::move(*itr));
						itr = secondBarrierBatch.erase(itr);
					}
					else
						++itr;
				}
			}
			else
			{
				// We should only ever have to do special resource initialization on the DIRECT
				// queue.
				assert(specialResourceInitializationEventArr.empty() && "ERROR: An attempt was made to perform special GPU resource initialization on a queue other than the DIRECT queue!");

				firstBarrierBatch.reserve(firstBarrierBatch.size() + secondBarrierBatch.size());

				for (auto&& delayedBarrier : secondBarrierBatch)
					firstBarrierBatch.push_back(std::move(delayedBarrier));
			}

			// Now, we can begin preparing our resources for the I_RenderPass. First, we
			// submit the first batch of D3D12 barriers.
			mContext->ResourceBarrier(std::span<const CD3DX12_RESOURCE_BARRIER>{ firstBarrierBatch });
			
			if constexpr (QueueType == GPUCommandQueueType::DIRECT)
			{
				// On the DIRECT queue, our next step is to perform special resource initialization.
				for (const auto& initializationEvent : specialResourceInitializationEventArr)
					mContext->PerformSpecialGPUResourceInitialization(*(initializationEvent.GPUResource));

				// Then, if there are any remaining barriers, we need to submit those. This might
				// happen, for instance, if a render target was just initialized, but now needs to be
				// put in a different resource state for an I_RenderPass.
				mContext->ResourceBarrier(std::span<const CD3DX12_RESOURCE_BARRIER>{ secondBarrierBatch });
			}

			// On all other queues, we don't need to worry about special resource initialization, since
			// they wouldn't support it, anyways. Thus, all of the barriers would have been submitted
			// in firstBarrierBatch.

			// Denote the GPUCommandContext as being useful if it has processed any GPUResourceEvents.
			if (!firstBarrierBatch.empty() || !specialResourceInitializationEventArr.empty() || !secondBarrierBatch.empty())
				mContext->MarkAsUseful();
		}
	}
}
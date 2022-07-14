module;
#include <vector>
#include <span>
#include <optional>
#include <functional>
#include <string>
#include <ranges>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.RenderPass;
import Brawler.D3D12.I_RenderPass;
import Brawler.SortedVector;
import Brawler.D3D12.FrameGraphResourceDependency;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.GPUCommandQueueContextType;
import Brawler.D3D12.GPUCommandContexts;
import Util.D3D12;
import Util.General;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.TextureSubResource;

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType, typename InputDataType>
		struct CallbackInfo
		{
			using CallbackType = std::function<void(GPUCommandQueueContextType<QueueType>&, const InputDataType&)>;
		};

		struct EmptyInput
		{};

		template <GPUCommandQueueType QueueType>
		struct CallbackInfo<QueueType, EmptyInput>
		{
			using CallbackType = std::function<void(GPUCommandQueueContextType<QueueType>&)>;
		};
	}
}

namespace Brawler
{
	namespace D3D12
	{
		class ReleaseModeRenderPassNameContainer
		{
		public:
			ReleaseModeRenderPassNameContainer() = default;

			static constexpr void SetRenderPassName(const std::string_view name)
			{}

			static consteval std::string_view GetRenderPassName()
			{
				return std::string_view{};
			}

			static constexpr void SetPIXEventColor(const Util::D3D12::PIXEventColor_T eventColor)
			{}

			static consteval Util::D3D12::PIXEventColor_T GetPIXEventColor()
			{
				return Util::D3D12::PIXEventColor_T{};
			}
		};

		class DebugModeRenderPassNameContainer
		{
		public:
			DebugModeRenderPassNameContainer() = default;

			void SetRenderPassName(const std::string_view name)
			{
				mRenderPassName = name;
			}

			const std::string_view GetRenderPassName() const
			{
				return mRenderPassName;
			}

			void SetPIXEventColor(const Util::D3D12::PIXEventColor_T eventColor)
			{
				mEventColor = eventColor;
			}

			Util::D3D12::PIXEventColor_T GetPIXEventColor() const
			{
				return (mEventColor.has_value() ? *mEventColor : Util::D3D12::PIX_EVENT_COLOR_CPU_GPU);
			}

		private:
			std::string_view mRenderPassName;
			std::optional<Util::D3D12::PIXEventColor_T> mEventColor;
		};

		template <Util::General::BuildMode BuildMode>
		struct RenderPassNameContainerSelector
		{
			using ContainerType = DebugModeRenderPassNameContainer;
		};

		template <>
		struct RenderPassNameContainerSelector<Util::General::BuildMode::RELEASE>
		{
			using ContainerType = ReleaseModeRenderPassNameContainer;
		};

		using CurrentRenderPassNameContainer = typename RenderPassNameContainerSelector<Util::General::GetBuildMode()>::ContainerType;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType, typename InputDataType = EmptyInput>
		class RenderPass final : public I_RenderPass<QueueType>, private CurrentRenderPassNameContainer
		{
		public:
			RenderPass() = default;

			RenderPass(const RenderPass& rhs) = delete;
			RenderPass& operator=(const RenderPass& rhs) = delete;

			RenderPass(RenderPass&& rhs) noexcept = default;
			RenderPass& operator=(RenderPass&& rhs) noexcept = default;

			bool RecordRenderPassCommands(GPUCommandQueueContextType<QueueType>& context) const override;

			template <typename InputDataType_>
				requires (!std::is_same_v<InputDataType, EmptyInput> && std::is_same_v<std::decay_t<InputDataType>, std::decay_t<InputDataType_>>)
			void SetInputData(InputDataType_&& inputData);

			void SetRenderPassCommands(typename CallbackInfo<QueueType, InputDataType>::CallbackType&& callback);

			/// <summary>
			/// Declares that this RenderPass instance will need access to the I_GPUResource instance
			/// specified by resource, and that its resource state must include that specified by
			/// requiredState.
			/// 
			/// This function serves two purposes:
			/// 
			///   1. It allows the FrameGraph to set up and optimize resource barriers before commands
			///      are recorded into a command list.
			/// 
			///   2. Resource dependencies within a RenderPass inform the FrameGraph of when a resource
			///      is being used, as well as how long it is being used for. This allows it to determine
			///      when a transient resource can yield its GPU memory to another transient resource via
			///      aliasing.
			/// 
			/// *NOTE*: It is valid to specify D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES for the
			/// SubResourceIndex field of resourceDependency. Doing so will mark all sub-resources for
			/// the relevant I_GPUResource as being used. However, if it is known that only a sub-set of
			/// the sub-resources of the I_GPUResource are going to be used, then specifying one
			/// FrameGraphResourceDependency instance for each sub-resource being used is generally more
			/// efficient.
			/// 
			/// *WARNING*: The behavior of the program is *UNDEFINED* if a RenderPass attempts to make
			/// use of an I_GPUResource instance (or a handle corresponding to said instance) without
			/// marking the resource as a dependency by calling this function.
			/// </summary>
			/// <param name="resourceDependency">
			/// - The FrameGraphResourceDependency which describes the I_GPUResource being used (or, to be
			///   more precise, the sub-resource(s) of the I_GPUResource), along with the required
			///   D3D12_RESOURCE_STATES which the sub-resource must be in. Note that the actual state of
			///   a sub-resource may be a combination of both this value and some other set of states,
			///   depending on how the sub-resource is used in other RenderPasses.
			/// </param>
			void AddResourceDependency(FrameGraphResourceDependency&& resourceDependency);

			void AddResourceDependency(I_BufferSubAllocation& bufferSubAllocation, const D3D12_RESOURCE_STATES requiredState);
			void AddResourceDependency(TextureSubResource& textureSubResource, const D3D12_RESOURCE_STATES requiredState);

			std::span<const FrameGraphResourceDependency> GetResourceDependencies() const override;

			void SetRenderPassName(const std::string_view name);
			const std::string_view GetRenderPassName() const override;

			void SetPIXEventColor(const Util::D3D12::PIXEventColor_T eventColor);
			Util::D3D12::PIXEventColor_T GetPIXEventColor() const;

		private:
			std::optional<typename CallbackInfo<QueueType, InputDataType>::CallbackType> mCmdRecordCallback;
			std::optional<InputDataType> mInputData;
			std::vector<FrameGraphResourceDependency> mResourceDependencyArr;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType, typename InputDataType>
		bool RenderPass<QueueType, InputDataType>::RecordRenderPassCommands(GPUCommandQueueContextType<QueueType>& context) const
		{
			// First, make sure that we actually have commands to record. Internally, the
			// Brawler Engine will occasionally create RenderPass instances without any
			// actual commands. This can happen, for instance, during the creation of a sync
			// point, or when creating a RenderPass to handle resource transitions which
			// a certain queue cannot.
			//
			// We declare this case to be unlikely because we expect the vast majority of
			// RenderPass instances to be those which go through only the DIRECT queue,
			// making it unnecessary to inject RenderPass instances for things like sync
			// points. Of course, we can always remove this in the future.
			if (!mCmdRecordCallback.has_value()) [[unlikely]]
				return false;
			
			// Next, set the valid resource dependencies for this RenderPass instance. These
			// are used by the GPUCommandContext in Debug builds to assert that only resources
			// which have a marked resource dependency are used.
			context.SetValidGPUResources(std::span<const FrameGraphResourceDependency>{ mResourceDependencyArr });

			// At long last, we can begin recording the user's commands. It certainly wasn't
			// an easy journey, but we made it, somehow.

			if constexpr (std::is_same_v<InputDataType, EmptyInput>)
			{
				// If InputDataType == EmptyInput, then the RenderPass instance has no
				// associated data to pass along.

				(*mCmdRecordCallback)(context);
			}
			else
			{
				// Otherwise, we need to make sure that the user has specified the input data
				// already.
				assert(mInputData.has_value() && "ERROR: A RenderPass was created with a custom InputDataType, but it was never provided any input data to send! (Use RenderPass::SetInputData() to do this.)");
				(*mCmdRecordCallback)(context, *mInputData);
			}

			return true;
		}
		
		template <GPUCommandQueueType QueueType, typename InputDataType>
		template <typename InputDataType_>
			requires (!std::is_same_v<InputDataType, EmptyInput>&& std::is_same_v<std::decay_t<InputDataType>, std::decay_t<InputDataType_>>)
		void RenderPass<QueueType, InputDataType>::SetInputData(InputDataType_&& inputData)
		{
			mInputData.emplace(std::forward<InputDataType_>(inputData));
		}

		template <GPUCommandQueueType QueueType, typename InputDataType>
		void RenderPass<QueueType, InputDataType>::SetRenderPassCommands(typename CallbackInfo<QueueType, InputDataType>::CallbackType&& callback)
		{
			mCmdRecordCallback = std::move(callback);
		}

		template <GPUCommandQueueType QueueType, typename InputDataType>
		void RenderPass<QueueType, InputDataType>::AddResourceDependency(FrameGraphResourceDependency&& resourceDependency)
		{
			const auto checkForExistingDependencyLambda = [this] (const I_GPUResource& resource, const std::uint32_t subResourceIndex)
			{
				assert(std::ranges::find_if(mResourceDependencyArr, [&resource, subResourceIndex] (const FrameGraphResourceDependency& dependency) { return (dependency.ResourcePtr == std::addressof(resource) && dependency.SubResourceIndex == subResourceIndex); }) == mResourceDependencyArr.end() &&
					"ERROR: An attempt was made to define a resource dependency for a sub-resource of an I_GPUResource more than once within the same RenderPass!");
			};

			assert(Util::D3D12::IsResourceStateValid(resourceDependency.RequiredState) && "ERROR: An invalid D3D12_RESOURCE_STATES value was provided in a call to RenderPass::AddResourceDependency()!");
			
			if (resourceDependency.SubResourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
			{
				const std::size_t subResourceCount = resourceDependency.ResourcePtr->GetSubResourceCount();
				
				if constexpr (Util::General::IsDebugModeEnabled())
				{
					for (auto i : std::views::iota(0u, subResourceCount))
						checkForExistingDependencyLambda(*(resourceDependency.ResourcePtr), i);

					assert(Util::D3D12::CanQueuePerformResourceTransition(QueueType, resourceDependency.RequiredState) && "ERROR: An attempt was made to declare a resource dependency in a RenderPass, but the queue to which said RenderPass will be sent does not support the required resource state! (For instance, the COMPUTE queue does not support the D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE state.)");
				}

				mResourceDependencyArr.reserve(mResourceDependencyArr.size() + subResourceCount);

				for (auto i : std::views::iota(0u, subResourceCount))
					mResourceDependencyArr.push_back(FrameGraphResourceDependency{
						.ResourcePtr = resourceDependency.ResourcePtr,
						.RequiredState = resourceDependency.RequiredState,
						.SubResourceIndex = i
					});
			}
			else
			{
				if constexpr (Util::General::IsDebugModeEnabled())
				{
					assert(resourceDependency.SubResourceIndex < resourceDependency.ResourcePtr->GetSubResourceCount() && "ERROR: An out-of-bounds sub-resource index was specified in a FrameGraphResourceDependency sent to RenderPass::AddResourceDependency()!");
					checkForExistingDependencyLambda(*(resourceDependency.ResourcePtr), resourceDependency.SubResourceIndex);
					assert(Util::D3D12::CanQueuePerformResourceTransition(QueueType, resourceDependency.RequiredState) && "ERROR: An attempt was made to declare a resource dependency in a RenderPass, but the queue to which said RenderPass will be sent does not support the required resource state! (For instance, the COMPUTE queue does not support the D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE state.)");
				}

				mResourceDependencyArr.push_back(std::move(resourceDependency));
			}
		}

		template <GPUCommandQueueType QueueType, typename InputDataType>
		void RenderPass<QueueType, InputDataType>::AddResourceDependency(I_BufferSubAllocation& bufferSubAllocation, const D3D12_RESOURCE_STATES requiredState)
		{
			AddResourceDependency(FrameGraphResourceDependency{
				.ResourcePtr = &(bufferSubAllocation.GetBufferResource()),
				.RequiredState = requiredState,
				.SubResourceIndex = 0
			});
		}

		template <GPUCommandQueueType QueueType, typename InputDataType>
		void RenderPass<QueueType, InputDataType>::AddResourceDependency(TextureSubResource& textureSubResource, const D3D12_RESOURCE_STATES requiredState)
		{
			AddResourceDependency(FrameGraphResourceDependency{
				.ResourcePtr = &(textureSubResource.GetGPUResource()),
				.RequiredState = requiredState,
				.SubResourceIndex = textureSubResource.GetSubResourceIndex()
			});
		}

		template <GPUCommandQueueType QueueType, typename InputDataType>
		std::span<const FrameGraphResourceDependency> RenderPass<QueueType, InputDataType>::GetResourceDependencies() const
		{
			return std::span<const FrameGraphResourceDependency>{ mResourceDependencyArr };
		}

		template <GPUCommandQueueType QueueType, typename InputDataType>
		void RenderPass<QueueType, InputDataType>::SetRenderPassName(const std::string_view name)
		{
			CurrentRenderPassNameContainer::SetRenderPassName(name);
		}

		template <GPUCommandQueueType QueueType, typename InputDataType>
		const std::string_view RenderPass<QueueType, InputDataType>::GetRenderPassName() const
		{
			return CurrentRenderPassNameContainer::GetRenderPassName();
		}

		template <GPUCommandQueueType QueueType, typename InputDataType>
		void RenderPass<QueueType, InputDataType>::SetPIXEventColor(const Util::D3D12::PIXEventColor_T eventColor)
		{
			CurrentRenderPassNameContainer::SetPIXEventColor(eventColor);
		}

		template <GPUCommandQueueType QueueType, typename InputDataType>
		Util::D3D12::PIXEventColor_T RenderPass<QueueType, InputDataType>::GetPIXEventColor() const
		{
			return CurrentRenderPassNameContainer::GetPIXEventColor();
		}
	}
}
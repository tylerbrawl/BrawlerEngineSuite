module;
#include <array>
#include <vector>
#include <tuple>
#include <memory>

export module Brawler.FrameGraphBlackboard;
import :FrameGraphBlackboardElementMap;
import Brawler.FrameGraphBlackboardElementID;
import Brawler.D3D12.I_GPUResource;
import Util.Engine;

export namespace Brawler
{
	class FrameGraphBlackboard
	{
	private:
		using TransientResourceContainer = std::vector<std::unique_ptr<D3D12::I_GPUResource>>;

	private:
		FrameGraphBlackboard();

	public:
		~FrameGraphBlackboard() = default;

		FrameGraphBlackboard(const FrameGraphBlackboard& rhs) = delete;
		FrameGraphBlackboard& operator=(const FrameGraphBlackboard& rhs) = delete;

		FrameGraphBlackboard(FrameGraphBlackboard&& rhs) noexcept = delete;
		FrameGraphBlackboard& operator=(FrameGraphBlackboard&& rhs) noexcept = delete;

		static FrameGraphBlackboard& GetInstance();

		template <FrameGraphBlackboardElementID ElementID>
		BlackboardElement_T<ElementID>& GetElement();

	private:
		void InitializeTransientResources();

		BlackboardElementTuple_T& GetCurrentBlackboardElementTuple();
		const BlackboardElementTuple_T& GetCurrentBlackboardElementTuple() const;

		TransientResourceContainer& GetCurrentTransientResourceContainer();
		const TransientResourceContainer& GetCurrentTransientResourceContainer() const;

	private:
		/// <summary>
		/// We actually want one tuple for each frame which is buffered by the Brawler
		/// Engine. That way, we can avoid the following race condition easily:
		/// 
		///   - Thread A calls FrameGraphBlackboard::InitializeTransientResources() to
		///     initialize the transient blackboard resources for Frame 2.
		/// 
		///   - During command list recording for Frame 1, Thread B calls
		///     FrameGraphBlackboard::GetElement() to retrieve the transient resources
		///     for Frame 1 belonging to a specific element of the blackboard. Depending
		///     on how the threads are scheduled, Thread B might end up with the resources
		///     for Frame 1, the resources for Frame 2, or some garbage state resulting from
		///     accessing a contended and unsynchronized memory location.
		/// 
		/// Doing this might also help reduce overall memory consumption. For instance, rather
		/// than requiring each RenderPass to capture a pointer to its required resources
		/// within its input data structure to ensure that the right resources are being used
		/// for the current frame, the elements can instead be safely obtained by accessing
		/// the static FrameGraphBlackboard instance within the RenderPass instance's command
		/// list recording callback function.
		/// </summary>
		std::array<BlackboardElementTuple_T, Util::Engine::MAX_FRAMES_IN_FLIGHT> mElementTupleArr;

		std::array<TransientResourceContainer, Util::Engine::MAX_FRAMES_IN_FLIGHT> mTransientResourceContainerArr;
	};
}

// ----------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <FrameGraphBlackboardElementID ElementID>
	BlackboardElement_T<ElementID>& FrameGraphBlackboard::GetElement()
	{
		return std::get<std::to_underlying(ElementID)>(GetCurrentBlackboardElementTuple());
	}
}
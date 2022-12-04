module;

export module Brawler.FrameGraphBlackboard;
import :FrameGraphBlackboardElementMap;
import Brawler.FrameGraphBlackboardElementID;

export namespace Brawler
{
	class FrameGraphBlackboard
	{
	private:
		FrameGraphBlackboard();

	public:
		~FrameGraphBlackboard() = default;

		FrameGraphBlackboard(const FrameGraphBlackboard& rhs) = delete;
		FrameGraphBlackboard& operator=(const FrameGraphBlackboard& rhs) = delete;

		FrameGraphBlackboard(FrameGraphBlackboard&& rhs) noexcept = delete;
		FrameGraphBlackboard& operator=(FrameGraphBlackboard&& rhs) noexcept = delete;

		static FrameGraphBlackboard& GetInstance();

	private:
		BlackboardElementTuple_T mElementTuple;
	};
}
module;
#include "DxDef.h"

module Tests.ResourceStateTrackingTest;
import Brawler.D3D12.FrameGraph;
import Util.Coroutine;

namespace Tests
{
	void RunResourceStateTrackingTests()
	{
		Brawler::D3D12::FrameGraph frameGraph{};
		frameGraph.Initialize();

		frameGraph.GenerateFrameGraph();
		frameGraph.SubmitFrameGraph();

		// The "proper" thing to do here would be to wait for the FrameGraph to finish
		// and then exit. However, in the actual Brawler Engine, we would have no real
		// need to explicitly wait for the FrameGraph to complete execution. (We might
		// need to do that internally to prevent destroying GPU resources too early,
		// but this is done automatically as a private function.)
		frameGraph.GenerateFrameGraph();

		Util::General::DebugBreak();
	}
}
module;

module Brawler.ComputeContext;
import Brawler.CommandListType;

namespace Brawler
{
	ComputeContext::ComputeContext() :
		I_RenderContext(Brawler::CommandListType::COMPUTE)
	{}
}
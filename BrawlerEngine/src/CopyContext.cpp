module;

module Brawler.CopyContext;
import Brawler.CommandListType;

namespace Brawler
{
	CopyContext::CopyContext() :
		I_RenderContext(Brawler::CommandListType::COPY)
	{}
}
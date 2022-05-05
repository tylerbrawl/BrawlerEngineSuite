module;

module Brawler.GraphicsContext;
import Brawler.CommandListType;

namespace Brawler
{
	GraphicsContext::GraphicsContext() :
		I_RenderContext(CommandListType::DIRECT)
	{}
}
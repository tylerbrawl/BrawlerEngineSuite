module;

export module Brawler.BorderlessWindowedModeParams;
import Brawler.Monitor;

export namespace Brawler
{
	struct BorderlessWindowedModeParams
	{
		const Monitor& AssociatedMonitor;
	};
}
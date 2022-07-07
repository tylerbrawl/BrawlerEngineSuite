module;

export module Brawler.WindowDisplayMode;

export namespace Brawler
{
	enum class WindowDisplayMode
	{
		WINDOWED,
		BORDERLESS_WINDOWED_FULL_SCREEN,
		FULL_SCREEN,

		MULTI_MONITOR,

		COUNT_OR_ERROR
	};
}
module;

export module Brawler.WindowStateID;

export namespace Brawler
{
	enum class WindowStateID
	{
		WINDOWED,
		BORDERLESS_WINDOWED_FULL_SCREEN,
		FULL_SCREEN,

		COUNT_OR_ERROR
	};
}
module;
#include <string>

export module Brawler.SettingID;

export namespace Brawler
{
	// When configuration options are saved to a file, they are written out in .ini format.
	// This enumeration is used to specify under which INI header a configuration option is
	// written to.
	//
	// You might wonder why we write the configuration options to an INI file, rather than
	// as raw binary. Consider things from the user's perspective. Let's say they make some
	// awful change to their settings which prevents them from changing it back within the
	// application.
	//
	// If the configuration file were written in binary, then the user would have two options:
	//
	//   1. Reverse-engineer the file format to manually change the setting back.
	//   2. Delete the configuration file to restore the default settings.
	//
	// By writing configuration options out to an INI file, we allow more advanced users to
	// fix their mistakes themselves without needing to take drastic measures. This does come
	// at the cost of longer save/load times, but I believe that this is a suitable tradeoff.
	// (Plus, I have empathy for all of the times that I needed to manually edit INI files. :))

	enum class SettingID
	{
		WINDOW_RESOLUTION_WIDTH,
		WINDOW_RESOLUTION_HEIGHT,
		RENDER_RESOLUTION_FACTOR,
		USE_FULLSCREEN,
		FRAME_RATE_LIMIT,

		COUNT_OR_ERROR
	};

	enum class SettingHeader
	{
		VIDEO
	};
}
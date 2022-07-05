module;
#include <compare>

export module Brawler.Manifest;
import Brawler.NZStringView;

export namespace Brawler
{
	namespace Manifest
	{
		// This is the name of the application. It is displayed in the caption of the application's
		// window.
		constexpr Brawler::NZWStringView APPLICATION_NAME{ L"Brawler Engine" };
	}
}
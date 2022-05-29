module;
#include <string_view>
#include <filesystem>

export module Brawler.BCAInfo;

export namespace Brawler
{
	struct BCAInfo
	{
		/// <summary>
		/// Specifies whether or not the corresponding file should be compressed within the BPK
		/// archive.
		/// </summary>
		bool DoNotCompress;
	};

	constexpr std::wstring_view BCA_INFO_FILE_NAME{ L".BCAINFO" };
	const std::filesystem::path BCA_INFO_FILE_PATH{ BCA_INFO_FILE_NAME };

	constexpr BCAInfo DEFAULT_BCA_INFO_VALUE{
		.DoNotCompress = false
	};
}
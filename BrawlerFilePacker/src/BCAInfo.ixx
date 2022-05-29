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

	constexpr BCAInfo DEFAULT_BCA_INFO_VALUE{
		.DoNotCompress = false
	};
}

namespace Brawler
{
	static const std::filesystem::path BCA_INFO_FILE_PATH{ BCA_INFO_FILE_NAME.data() };
}

export namespace Brawler
{
	const std::filesystem::path& GetBCAInfoFilePath()
	{
		return BCA_INFO_FILE_PATH;
	}
}
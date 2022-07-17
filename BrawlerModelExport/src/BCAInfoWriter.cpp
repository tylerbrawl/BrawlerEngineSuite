module;
#include <filesystem>
#include <vector>
#include <fstream>
#include <ranges>
#include <cassert>
#include <cwchar>

module Brawler.BCAInfoWriter;
import Brawler.NZStringView;

namespace Brawler
{
	void BCAInfoWriter::AddEntry(BCAInfoEntry&& entry)
	{
		mEntryArr.push_back(std::move(entry));
	}

	void BCAInfoWriter::SerializeBCAInfoFile(const std::filesystem::path& outputDirectoryPath) const
	{
		if (mEntryArr.empty()) [[unlikely]]
			return;

		assert(std::filesystem::is_directory(outputDirectoryPath) && "ERROR: The std::filesystem::path specified in a call to BCAInfoWriter::SerializeBCAInfoFile() should refer to a directory!");
		assert(std::wcscmp(outputDirectoryPath.filename().c_str(), L".BCAINFO") && "ERROR: The std::filesystem::path specified in a call to BCAInfoWriter::SerializeBCAInfoFile() should not include the name of the output file!");

		// Never forget to create the parent directory!
		std::filesystem::create_directories(outputDirectoryPath.parent_path());
		std::ofstream outputBCAInfoFileStream{ std::filesystem::path{ outputDirectoryPath / L".BCAINFO" }, std::ios::out };

		outputBCAInfoFileStream << mEntryArr[0].GetEntryText();

		for (const auto& entry : mEntryArr | std::views::drop(1))
			outputBCAInfoFileStream << "\n\n" << entry.GetEntryText();
	}
}
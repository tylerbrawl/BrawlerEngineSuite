module;
#include <filesystem>
#include <vector>

export module Brawler.BCAInfoWriter;
import Brawler.BCAInfoEntry;

export namespace Brawler
{
	class BCAInfoWriter
	{
	public:
		BCAInfoWriter() = default;

		BCAInfoWriter(const BCAInfoWriter& rhs) = delete;
		BCAInfoWriter& operator=(const BCAInfoWriter& rhs) = delete;

		BCAInfoWriter(BCAInfoWriter&& rhs) noexcept = default;
		BCAInfoWriter& operator=(BCAInfoWriter&& rhs) noexcept = default;

		void AddEntry(BCAInfoEntry&& entry);

		void SerializeBCAInfoFile(const std::filesystem::path& outputDirectoryPath) const;

	private:
		std::vector<BCAInfoEntry> mEntryArr;
	};
}
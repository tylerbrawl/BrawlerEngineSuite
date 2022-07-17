module;
#include <filesystem>
#include <string>

export module Brawler.BCAInfoEntry;

export namespace Brawler
{
	class BCAInfoEntry
	{
	public:
		BCAInfoEntry() = default;
		explicit BCAInfoEntry(std::filesystem::path&& associatedFilePath);

		BCAInfoEntry(const BCAInfoEntry& rhs) = delete;
		BCAInfoEntry& operator=(const BCAInfoEntry& rhs) = delete;

		BCAInfoEntry(BCAInfoEntry&& rhs) noexcept = default;
		BCAInfoEntry& operator=(BCAInfoEntry&& rhs) noexcept = default;

		void SetCompressionStatus(const bool enableCompression);

		const std::filesystem::path& GetAssociatedFilePath() const;
		std::string GetEntryText() const;

	private:
		std::filesystem::path mFilePath;

		// There aren't many options available in BCAINFO files yet, so something
		// simple like one member variable per option is fine. If more get added,
		// however, then we might want to consider creating some class to represent
		// options within a BCAINFO (e.g., BCAInfoOption or BCAInfoField).

		bool mDoNotCompress;
	};
}
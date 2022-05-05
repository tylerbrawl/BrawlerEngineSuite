module;
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>

export module Brawler.BPKFactory;
import Brawler.BCAArchive;

export namespace Brawler
{
	struct AssetCompilerContext;
}

export namespace Brawler
{
	class BPKFactory
	{
	public:
		explicit BPKFactory(std::vector<std::unique_ptr<BCAArchive>>&& bcaArchiveArr);

		void CreateBPKArchive(const AssetCompilerContext& context) const;

	private:
		void WriteBPKFile(const std::filesystem::path& bpkOutputPath) const;

		template <typename VersionedBPKFileHeader>
		VersionedBPKFileHeader CreateVersionedBPKFileHeader() const;

		template <typename VersionedBPKFileHeader>
		void WriteTableOfContents(std::ofstream& bpkFileStream) const;

	private:
		std::vector<std::unique_ptr<BCAArchive>> mBCAArchiveArr;
	};
}
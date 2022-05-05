module;
#include <memory>
#include <vector>
#include <mutex>

export module Brawler.BCALinker;
import Brawler.BCAArchive;

export namespace Brawler
{
	struct AssetCompilerContext;
}

export namespace Brawler
{
	class BCALinker
	{
	public:
		BCALinker();

		BCALinker(const BCALinker& rhs) = delete;
		BCALinker& operator=(const BCALinker& rhs) = delete;

		BCALinker(BCALinker&& rhs) noexcept = default;
		BCALinker& operator=(BCALinker&& rhs) noexcept = default;

		void AddBCAArchive(std::unique_ptr<BCAArchive>&& bcaArchive);
		void PackBCAArchives(const AssetCompilerContext& context);

	private:
		/// <summary>
		/// Checks for hash collisions between the submitted BCA files. If a
		/// hash collision is detected, then file names will need to be changed
		/// in order to pack them into a BPK archive.
		/// </summary>
		/// <returns>
		/// The function returns true if no hash collisions were found and false
		/// otherwise.
		/// </returns>
		bool CheckForHashCollisions() const;

	private:
		std::vector<std::unique_ptr<BCAArchive>> mBCAArchiveArr;

		// We need to have the mutex protect both the std::unordered_map and the
		// std::vector instances contained within it. Otherwise, we will face
		// race conditions.
		std::mutex mutable mCritSection;
	};
}
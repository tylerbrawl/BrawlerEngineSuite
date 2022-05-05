module;
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <filesystem>
#include <stdexcept>

module Brawler.BCALinker;
import Brawler.StringHasher;
import Brawler.BCAMetadata;
import Brawler.JobSystem;
import Brawler.BPKFactory;
import Brawler.AssetCompilerContext;
import Util.Win32;

namespace Brawler
{
	BCALinker::BCALinker() :
		mBCAArchiveArr(),
		mCritSection()
	{}

	void BCALinker::AddBCAArchive(std::unique_ptr<BCAArchive>&& bcaArchive)
	{
		// Add the BCAArchive to the end of the BCAArchive std::vector.
		// At the time of writing this, this is done with a critical section
		// protecting the container.
		// 
		// Benchmarking will help us to determine if this is a performance issue.
		// Personally, I don't see this as being too big of an issue; the largest
		// bottleneck for performance will by far be compressing asset data for
		// Release builds.

		std::scoped_lock<std::mutex> lock{ mCritSection };
		mBCAArchiveArr.push_back(std::move(bcaArchive));
	}

	void BCALinker::PackBCAArchives(const AssetCompilerContext& context)
	{
		// At this point, all of the other threads have finished accessing the
		// mBCAArchiveArr; thus, we don't need to use the critical section. (To
		// be fair, however, since only one thread would be calling this, locking
		// the critical section would still be a super fast operation.)

		{
			Util::Win32::WriteFormattedConsoleMessage(L"Checking for asset path hash collisions...");

			if (!CheckForHashCollisions())
				throw std::runtime_error{ "ERROR: There were hash collisions detected between asset path names. Refer to the command prompt output to see which path conflicts need to be resolved." };
		}
		
		{
			Util::Win32::WriteFormattedConsoleMessage(L"Building .bpk archive file...");
			
			BPKFactory bpkFactory{ std::move(mBCAArchiveArr) };
			bpkFactory.CreateBPKArchive(context);
		}
		
	}

	bool BCALinker::CheckForHashCollisions() const
	{
		std::unordered_map<std::uint64_t, std::filesystem::path> hashAssetPathMap{};
		bool collisionFound = false;

		for (const auto& bcaArchivePtr : mBCAArchiveArr)
		{
			const std::uint64_t assetPathHash = bcaArchivePtr->GetMetadata().SourceAssetDirectoryHash;

			if (hashAssetPathMap.contains(assetPathHash))
			{
				// Uh oh... We have a hash collision. In this case, our only option is to rename
				// the asset files. (Either that, or we need to change our hashing algorithm,
				// but that probably isn't going to happen.)
				const std::wstring conflictMsg{ L"Hash Collision Detected: " + hashAssetPathMap[assetPathHash].wstring() + L" | " + bcaArchivePtr->GetAssetDataPath().wstring() };
				Util::Win32::WriteFormattedConsoleMessage(conflictMsg, Util::Win32::ConsoleFormat::CRITICAL_FAILURE);

				collisionFound = true;
			}
			else
				hashAssetPathMap[assetPathHash] = bcaArchivePtr->GetAssetDataPath();
		}

		return !collisionFound;
	}
}
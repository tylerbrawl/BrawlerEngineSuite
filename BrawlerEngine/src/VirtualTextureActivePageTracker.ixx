module;
#include <unordered_map>
#include <optional>
#include <span>

export module Brawler.VirtualTextureActivePageTracker;
import Brawler.GlobalTexturePageIdentifier;
import Brawler.GeneralHash;

export namespace Brawler
{
	class VirtualTextureActivePageTracker
	{
	public:
		struct TrackedPage
		{
			std::uint32_t LogicalMipLevel;
			std::uint32_t LogicalPageXCoordinate;
			std::uint32_t LogicalPageYCoordinate;
		};

	public:
		VirtualTextureActivePageTracker() = default;

		VirtualTextureActivePageTracker(const VirtualTextureActivePageTracker& rhs) = delete;
		VirtualTextureActivePageTracker& operator=(const VirtualTextureActivePageTracker& rhs) = delete;

		VirtualTextureActivePageTracker(VirtualTextureActivePageTracker&& rhs) noexcept = default;
		VirtualTextureActivePageTracker& operator=(VirtualTextureActivePageTracker&& rhs) noexcept = default;

		void UpdateTrackedPage(const TrackedPage trackedPage, const GlobalTexturePageIdentifier storagePageIdentifier);
		void RemoveTrackedPage(const TrackedPage trackedPage);

		std::optional<GlobalTexturePageIdentifier> GetStoragePageIdentifier(const TrackedPage trackedPage) const;

	private:
		std::unordered_map<TrackedPage, GlobalTexturePageIdentifier> mPageStorageMap;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------------

export namespace std
{
	template <>
	struct hash<Brawler::VirtualTextureActivePageTracker::TrackedPage>
	{
	private:
		using KeyType = Brawler::VirtualTextureActivePageTracker::TrackedPage;

	public:
		std::size_t operator()(const KeyType& key) const noexcept
		{
			const Brawler::GeneralHash<KeyType> hasher{};
			return hasher(key);
		}
	};
}
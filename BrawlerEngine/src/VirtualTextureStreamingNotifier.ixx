module;
#include <cstdint>

export module Brawler.VirtualTextureStreamingNotifier;
import Brawler.VirtualTextureLogicalPage;

export namespace Brawler
{
	class VirtualTextureStreamingNotifier
	{
	protected:
		VirtualTextureStreamingNotifier() = default;
		explicit VirtualTextureStreamingNotifier(const VirtualTextureLogicalPage& logicalPage);

	public:
		virtual ~VirtualTextureStreamingNotifier();

		VirtualTextureStreamingNotifier(const VirtualTextureStreamingNotifier& rhs) = delete;
		VirtualTextureStreamingNotifier& operator=(const VirtualTextureStreamingNotifier& rhs) = delete;

		VirtualTextureStreamingNotifier(VirtualTextureStreamingNotifier&& rhs) noexcept;
		VirtualTextureStreamingNotifier& operator=(VirtualTextureStreamingNotifier&& rhs) noexcept;

		const VirtualTextureLogicalPage& GetLogicalPage() const;

	protected:
		void SetVirtualTextureFirstUseableFrameNumber(const std::uint64_t frameNumber) const;

	private:
		void TryDecrementStreamingRequestCount();

	private:
		VirtualTextureLogicalPage mLogicalPage;
	};
}
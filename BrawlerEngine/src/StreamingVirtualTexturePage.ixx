module;

export module Brawler.StreamingVirtualTexturePage;
import Brawler.VirtualTexture;

export namespace Brawler
{
	class StreamingVirtualTexturePage
	{
	public:
		StreamingVirtualTexturePage() = default;

		StreamingVirtualTexturePage(const StreamingVirtualTexturePage& rhs) = delete;
		StreamingVirtualTexturePage& operator=(const StreamingVirtualTexturePage& rhs) = delete;

		StreamingVirtualTexturePage(StreamingVirtualTexturePage&& rhs) noexcept = default;
		StreamingVirtualTexturePage& operator=(StreamingVirtualTexturePage&& rhs) noexcept = default;

	private:

	};
}
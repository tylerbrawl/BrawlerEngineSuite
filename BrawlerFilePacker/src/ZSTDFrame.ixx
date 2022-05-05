module;
#include <vector>
#include <span>
#include <iostream>

export module Brawler.ZSTDFrame;

export
{
	namespace Brawler
	{
		class ZSTDFrame;
	}

	std::ostream& operator<<(std::ostream& lhs, const Brawler::ZSTDFrame& rhs);
}

export namespace Brawler
{
	class ZSTDFrame
	{
	private:
		friend std::ostream& ::operator<<(std::ostream& lhs, const ZSTDFrame& rhs);

	public:
		ZSTDFrame();
		explicit ZSTDFrame(std::vector<std::uint8_t>&& frameByteArr);

		std::span<const std::uint8_t> GetByteArray() const;
		bool IsEmpty() const;

	private:
		std::vector<std::uint8_t> mFrameByteArr;
	};
}
module;
#include <vector>
#include <span>
#include <iostream>

module Brawler.ZSTDFrame;

std::ostream& operator<<(std::ostream& lhs, const Brawler::ZSTDFrame& rhs)
{
	lhs.write(reinterpret_cast<const char*>(rhs.mFrameByteArr.data()), rhs.mFrameByteArr.size());

	return lhs;
}

namespace Brawler
{
	ZSTDFrame::ZSTDFrame() :
		mFrameByteArr()
	{}
	
	ZSTDFrame::ZSTDFrame(std::vector<std::uint8_t>&& frameByteArr) :
		mFrameByteArr(std::move(frameByteArr))
	{}

	std::span<const std::uint8_t> ZSTDFrame::GetByteArray() const
	{
		return mFrameByteArr;
	}

	bool ZSTDFrame::IsEmpty() const
	{
		return mFrameByteArr.empty();
	}
}
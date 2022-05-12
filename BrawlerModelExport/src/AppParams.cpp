module;
#include <string>
#include <vector>
#include <filesystem>
#include <cassert>

module Brawler.AppParams;
import Util.General;

namespace Brawler
{
	void AppParams::SetModelName(const std::string_view modelName)
	{
		mModelName = Util::General::StringToWString(modelName);
	}

	const std::wstring_view AppParams::GetModelName() const
	{
		return mModelName;
	}

	void AppParams::SetLODCount(const std::uint32_t numLODFiles)
	{
		assert(mInputLODFilePathArr.empty());
		mInputLODFilePathArr.resize(static_cast<std::size_t>(numLODFiles));
	}

	void AppParams::AddLODFilePath(const std::uint32_t lodLevel, const std::string_view lodFilePath)
	{
		const std::size_t largeLODLevel = static_cast<std::size_t>(lodLevel);
		
		assert(largeLODLevel < mInputLODFilePathArr.size());
		mInputLODFilePathArr[largeLODLevel] = std::filesystem::path{ lodFilePath };
	}

	std::uint32_t AppParams::GetLODCount() const
	{
		return static_cast<std::uint32_t>(mInputLODFilePathArr.size());
	}

	const std::filesystem::path& AppParams::GetLODFilePath(const std::uint32_t lodLevel) const
	{
		assert(lodLevel < mInputLODFilePathArr.size());
		return mInputLODFilePathArr[lodLevel];
	}

	void AppParams::SetRootOutputDirectory(const std::string_view rootOutputDir)
	{
		mRootOutputDirectory = std::filesystem::path{ rootOutputDir };
	}

	const std::filesystem::path& AppParams::GetRootOutputDirectory() const
	{
		return mRootOutputDirectory;
	}
}
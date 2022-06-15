module;
#include <string>
#include <vector>
#include <filesystem>
#include <span>
#include <cassert>

module Brawler.LaunchParams;
import Util.General;

namespace Brawler
{
	void LaunchParams::SetModelName(const std::string_view modelName)
	{
		mModelName = Util::General::StringToWString(modelName);
	}

	const std::wstring_view LaunchParams::GetModelName() const
	{
		return mModelName;
	}

	void LaunchParams::SetLODCount(const std::uint32_t numLODFiles)
	{
		assert(mInputLODFilePathArr.empty());
		mInputLODFilePathArr.resize(static_cast<std::size_t>(numLODFiles));
	}

	void LaunchParams::AddLODFilePath(const std::uint32_t lodLevel, const std::string_view lodFilePath)
	{
		const std::size_t largeLODLevel = static_cast<std::size_t>(lodLevel);
		
		assert(largeLODLevel < mInputLODFilePathArr.size());
		mInputLODFilePathArr[largeLODLevel] = std::filesystem::path{ lodFilePath };
	}

	std::uint32_t LaunchParams::GetLODCount() const
	{
		return static_cast<std::uint32_t>(mInputLODFilePathArr.size());
	}

	std::span<const std::filesystem::path> LaunchParams::GetLODFilePaths() const
	{
		assert(!mInputLODFilePathArr.empty());
		return std::span<const std::filesystem::path>{ mInputLODFilePathArr };
	}

	const std::filesystem::path& LaunchParams::GetLODFilePath(const std::uint32_t lodLevel) const
	{
		assert(lodLevel < mInputLODFilePathArr.size());
		return mInputLODFilePathArr[lodLevel];
	}

	void LaunchParams::SetRootOutputDirectory(const std::string_view rootOutputDir)
	{
		mRootOutputDirectory = std::filesystem::path{ rootOutputDir };
	}

	const std::filesystem::path& LaunchParams::GetRootOutputDirectory() const
	{
		return mRootOutputDirectory;
	}
}
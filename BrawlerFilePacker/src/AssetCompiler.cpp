module;
#include <string>
#include <filesystem>
#include <ranges>
#include <memory>
#include <format>
#include <cwctype>

module Brawler.AssetCompiler;
import Brawler.AppParams;
import Brawler.AssetCompilerContext;
import Util.General;
import Brawler.JobSystem;
import Brawler.BCAArchive;
import Brawler.BCALinker;
import Util.Win32;
import Brawler.BCAInfo;
import Brawler.BCAInfoDatabase;
import Brawler.BCAInfoParsing.BCAInfoParser;

namespace Brawler
{
	AssetCompiler::AssetCompiler() :
		mBCALinker()
	{}

	void AssetCompiler::BeginAssetCompilationPipeline(const AssetCompilerContext& context)
	{
		EnsureDirectoryValidity(context);

		Util::Win32::WriteFormattedConsoleMessage("Creating .bca archive files...\n");
		CompileAssets(context);
		
		Util::Win32::WriteFormattedConsoleMessage("\nAll BCA archives were successfully created. Creating .bpk archive file...");
		mBCALinker.PackBCAArchives(context);

		Util::Win32::WriteFormattedConsoleMessage("[BUILD SUCCESSFUL]", Util::Win32::ConsoleFormat::SUCCESS);
	}

	void AssetCompiler::EnsureDirectoryValidity(const AssetCompilerContext& context) const
	{
		// First, verify that the data directory exists.
		if (!std::filesystem::exists(context.RootDataDirectory)) [[unlikely]]  // I just learned about this yesterday at the time of writing this! Yay!
		{
			std::string errMsg{ "ERROR: The input data directory " + context.RootDataDirectory.string() + " does not exist!" };
			throw std::runtime_error{ std::move(errMsg) };
		}

		// The root output directory should have two sub-folders:
		//
		//   - Asset Cache
		//   - Compiled Packages
		//
		// Thankfully, std::filesystem makes handling space characters easy, so we
		// don't have to worry about handling Windows file paths ourselves.

		const std::filesystem::path assetCachePath{ context.RootOutputDirectory / L"Asset Cache" };
		std::error_code fileCreationErrorCode{};

		std::filesystem::create_directories(assetCachePath, fileCreationErrorCode);

		if (fileCreationErrorCode) [[unlikely]]
			throw std::runtime_error{ "ERROR: The asset cache directory " + assetCachePath.string() + " could not be created for the following reason: " + fileCreationErrorCode.message() };

		const std::filesystem::path compiledPackagesPath{ context.RootOutputDirectory / L"Compiled Packages" };

		std::filesystem::create_directories(compiledPackagesPath, fileCreationErrorCode);

		if (fileCreationErrorCode) [[unlikely]]
			throw std::runtime_error{ "ERROR: The compiled packages directory " + compiledPackagesPath.string() + " could not be created for the following reason: " + fileCreationErrorCode.message() };
	}

	void AssetCompiler::CompileAssets(const AssetCompilerContext& context)
	{
		// For every file in the data directory, ...
		const auto fileFilter = [] (const std::filesystem::directory_entry& dirEntry) -> bool
		{
			return !(std::filesystem::is_directory(dirEntry));
		};

		std::filesystem::recursive_directory_iterator dirItr{ context.RootDataDirectory };

		Brawler::JobGroup bcaCreationJobGroup{};
		bcaCreationJobGroup.Reserve(std::ranges::count_if(dirItr, fileFilter));

		dirItr = std::filesystem::recursive_directory_iterator{ context.RootDataDirectory };
		for (const auto& fileDirectory : dirItr | std::views::filter(fileFilter))
		{
			// ... construct a Brawler::Job which is responsible for creating its
			// corresponding .bca file, unless the file is a .BCAINFO file. In that
			// case, we use it to initialize BCAInfo structures for the other
			// files.
			std::filesystem::path filePath{ fileDirectory.path() };
			const bool isBCAInfoFile = [&filePath] ()
			{
				std::wstring fileExtensionStr{ filePath.filename().wstring() };
				std::ranges::transform(fileExtensionStr, fileExtensionStr.begin(), [] (const wchar_t c) { return std::towupper(c); });

				return fileExtensionStr == GetBCAInfoFilePath().c_str();
			}();

			if (isBCAInfoFile)
			{
				Util::Win32::WriteFormattedConsoleMessage(std::format(L".BCAINFO File Detected: \"{}\"", filePath.c_str()));
				
				// Register the .BCAINFO with the BCAInfoDatabase. We need to do this *BEFORE* any of the CPU jobs
				// are submitted.
				BCAInfoDatabase::GetInstance().RegisterBCAInfoFilePath(filePath);

				bcaCreationJobGroup.AddJob([filePath] () mutable
				{
					BCAInfoParsing::BCAInfoParser bcaInfoParser{};

					const bool parsingSuccessful = bcaInfoParser.ParseBCAInfoFile(std::move(filePath));
					bcaInfoParser.UpdateBCAInfoDatabase();
					
					if(!parsingSuccessful) [[unlikely]]
					{
						bcaInfoParser.PrintErrorMessages();
						throw std::runtime_error{ "" };
					}
				});
			}
			else
			{
				bcaCreationJobGroup.AddJob([this, &context, fileDirectory] ()
				{
					std::unique_ptr<BCAArchive> bcaArchive{ std::make_unique<BCAArchive>(context, std::filesystem::path{ fileDirectory.path() }) };
					bcaArchive->InitializeArchiveData();

					mBCALinker.AddBCAArchive(std::move(bcaArchive));
				});
			}
		}
			
		
		bcaCreationJobGroup.ExecuteJobs();
	}
}
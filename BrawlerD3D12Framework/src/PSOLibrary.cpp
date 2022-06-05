module;
#include <atomic>
#include <filesystem>
#include <span>
#include <format>
#include <cassert>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <cwctype>
#include "DxDef.h"

module Brawler.D3D12.PSODatabase;
import Util.General;
import Util.Engine;
import Util.Win32;
import Brawler.MappedFileView;
import Brawler.JobSystem;
import Brawler.D3D12.PipelineEnums;
import Brawler.NZStringView;
import Brawler.EngineConstants;

namespace
{
	static constexpr Brawler::NZWStringView PSO_LIBRARY_FILE_EXTENSION{ L".bpl" };
	
	static const std::filesystem::path psoLibraryParentDirectory{ std::filesystem::current_path() / L"Data" };
	static const std::filesystem::path psoLibraryFilePath{ psoLibraryParentDirectory / Brawler::D3D12::PSO_CACHE_FILE_NAME.C_Str() };

	// By using __forceinline, we can "unroll" the template recursion.
	template <Brawler::PSOs::PSOID PSOIdentifier>
	__forceinline void SerializePSO(Brawler::D3D12PipelineLibrary& psoLibrary)
	{
		if constexpr (PSOIdentifier != Brawler::PSOs::PSOID::COUNT_OR_ERROR)
		{
			constexpr Brawler::NZWStringView UNIQUE_PSO_NAME{ Brawler::PSOs::GetUniquePSOName<PSOIdentifier>() };
			Brawler::D3D12PipelineState& currPSO{ Brawler::D3D12::PSODatabase::GetInstance().GetPipelineState<PSOIdentifier>() };

			Util::General::CheckHRESULT(psoLibrary.StorePipeline(UNIQUE_PSO_NAME.C_Str(), &currPSO));

			constexpr Brawler::PSOs::PSOID NEXT_PSO_ID = static_cast<Brawler::PSOs::PSOID>(std::to_underlying(PSOIdentifier) + 1);
			SerializePSO<NEXT_PSO_ID>(psoLibrary);
		}
	}
}

namespace Brawler
{
	namespace D3D12
	{
		void PSOLibrary::Initialize()
		{
			assert(!PSO_CACHE_FILE_NAME.Empty() && "ERROR: An empty file path was specified in the extern engine constant Brawler::D3D12::PSO_CACHE_FILE_NAME!");
			
			// Ensure that a standardized file extension is being used in Debug builds.
			if constexpr (Util::General::IsDebugModeEnabled())
			{
				const std::filesystem::path psoLibraryFileExtension{ psoLibraryFilePath.extension() };

				std::wstring extensionStr{ psoLibraryFileExtension.wstring() };
				std::ranges::transform(extensionStr, extensionStr.begin(), [] (const wchar_t currChar) { return std::towlower(currChar); });

				assert(extensionStr == PSO_LIBRARY_FILE_EXTENSION && "ERROR: A file with an invalid file extension was specified in a call to PSOLibrary::Initialize()! (All PSO library files must have the \".bpl\" extension.)");
			}
			
			std::error_code errorCode{};

			const bool fileExists = std::filesystem::exists(psoLibraryFilePath, errorCode);
			Util::General::CheckErrorCode(errorCode);

			// Exit early if the PSO library does not exist in the file system.
			if (!fileExists) [[unlikely]]
			{
				Util::Win32::WriteFormattedConsoleMessage(std::format(LR"(No existing PSO cache was found in the expected directory "{}." A full compilation of all PSOs is necessary.)", psoLibraryParentDirectory.c_str()));
				
				mNeedsSerialization.store(true, std::memory_order::relaxed);
				return;
			}

			const bool isDirectory = std::filesystem::is_directory(psoLibraryFilePath, errorCode);
			Util::General::CheckErrorCode(errorCode);

			// This shouldn't happen in the common case, but if somehow a directory was created
			// with the same name, then we'll also exit early. We'll be able to delete it later.
			if (isDirectory) [[unlikely]]
			{
				mNeedsSerialization.store(true, std::memory_order::relaxed);
				return;
			}

			const auto fileSize = std::filesystem::file_size(psoLibraryFilePath, errorCode);
			Util::General::CheckErrorCode(errorCode);

			mLibraryFileView = MappedFileView<FileAccessMode::READ_ONLY>{ psoLibraryFilePath, MappedFileView<FileAccessMode::READ_ONLY>::ViewParams{
				.FileOffsetInBytes = 0,
				.ViewSizeInBytes = fileSize
			} };

			if (!mLibraryFileView.IsValidView()) [[unlikely]]
			{
				mNeedsSerialization.store(true, std::memory_order::relaxed);
				return;
			}

			const std::span<const std::byte> mappedPipelineLibraryData{ mLibraryFileView.GetMappedData() };

			Microsoft::WRL::ComPtr<ID3D12PipelineLibrary> oldPipelineLibrary{};
			HRESULT hr = Util::Engine::GetD3D12Device().CreatePipelineLibrary(mappedPipelineLibraryData.data(), mappedPipelineLibraryData.size_bytes(), IID_PPV_ARGS(&oldPipelineLibrary));

			switch (hr)
			{
			// S_OK: The ID3D12PipelineLibrary instance was successfully created.
			case S_OK: [[likely]]
				break;

			// D3D12_ERROR_DRIVER_VERSION_MISMATCH: The PSO library was created with an older version of either the D3D12 runtime or a graphics driver.
			// D3D12_ERROR_ADAPTER_NOT_FOUND: The PSO library was created for a different video adapter.
			//
			// We list these errors before other types of errors because they are more likely to occur.
			case D3D12_ERROR_DRIVER_VERSION_MISMATCH: [[fallthrough]];
			case D3D12_ERROR_ADAPTER_NOT_FOUND:
			{
				Util::Win32::WriteFormattedConsoleMessage(L"The detected PSO cache was created with a different hardware configuration, or with an older version of the D3D12 runtime. A full re-compilation of all PSOs is necessary.");

				mNeedsSerialization.store(true, std::memory_order::relaxed);
				return;
			}

			// E_INVALIDARG: The cached PSO library data blob is either corrupted or unrecognizable.
			case E_INVALIDARG: [[unlikely]]
			{
				Util::Win32::WriteFormattedConsoleMessage(L"The PSO cache was somehow corrupted. A full re-compilation of all PSOs is necessary.");

				mNeedsSerialization.store(true, std::memory_order::relaxed);
				return;
			}

			// DXGI_ERROR_UNSUPPORTED: Either the OS version or the graphics driver is outdated.
			case DXGI_ERROR_UNSUPPORTED: [[unlikely]]
			{
				throw std::runtime_error{ "ERROR: The graphics driver of this device is too old to support D3D12 PSO libraries. Please update your graphics drivers before re-launching the application." };

				// Don't set mNeedsSerialization to true if we get this error. We wouldn't be able to
				// serialize a D3D12PipelineLibrary instance, anyways.

				return;
			}

			default: [[unlikely]]
			{
				Util::General::CheckHRESULT(hr);
				return;
			}
			}

			hr = oldPipelineLibrary.As(&mPipelineLibrary);

			if (FAILED(hr)) [[unlikely]]
				mNeedsSerialization.store(true, std::memory_order::relaxed);
		}

		void PSOLibrary::SerializePSOLibraryAsync() const
		{
			// Do not serialize the PSO library if we do not need to. In most cases, we won't.
			if (!mNeedsSerialization.load(std::memory_order::relaxed)) [[likely]]
				return;

			// Create the CPU job with a low priority. It won't need to be completed quickly.
			Brawler::JobGroup psoCacheSerializationGroup{ Brawler::JobPriority::LOW };

			psoCacheSerializationGroup.AddJob([this] ()
			{
				BeginPSOLibrarySerialization();
			});

			psoCacheSerializationGroup.ExecuteJobsAsync();
		}

		void PSOLibrary::BeginPSOLibrarySerialization() const
		{
			// The procedure for serializing the PSO library is analogous to creating an in-memory
			// buffer and copying data to it. It is a two-step process:
			//
			//   1. Create the destination file in the file system and re-size it to match the size
			//      of the serialized D3D12PipelineLibrary instance. This is analogous to doing an
			//      malloc() for a dynamically-sized buffer.
			//
			//   2. Use memory-mapped I/O to write the PSO library directly to the file system. This
			//      is analogous to doing a memcpy() into the aforementioned buffer.
			//
			// This is much more efficient than actually writing the data into an in-memory buffer
			// and then serializing the contents of that buffer with, say, std::ofstream.
			
			Microsoft::WRL::ComPtr<Brawler::D3D12PipelineLibrary> currPipelineLibrary{};
			
			{
				Microsoft::WRL::ComPtr<ID3D12PipelineLibrary> oldPipelineLibrary{};
				Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreatePipelineLibrary(nullptr, 0, IID_PPV_ARGS(&oldPipelineLibrary)));


				Util::General::CheckHRESULT(oldPipelineLibrary.As(&currPipelineLibrary));
			}

			// The D3D12 PSO library does not support overwriting PSOs. This means that any
			// time we want to make a change to an existing PSO, we have to either bloat the
			// library with unused PSOs or create a new one from scratch. It seems like the
			// latter is the lesser of two evils, especially since the serialization is done
			// asynchronously.

			SerializePSO<static_cast<Brawler::PSOs::PSOID>(0)>(*(currPipelineLibrary.Get()));

			// Create the parent directory of the PSO library. We won't be able to open a
			// file stream if we don't make sure that it exists.
			std::error_code errorCode{};

			std::filesystem::create_directories(psoLibraryParentDirectory, errorCode);
			Util::General::CheckErrorCode(errorCode);

			// ID3D12PipelineLibrary::Serialize() requires us to provide a pointer to a location
			// which receives the serialized PSO library data. It is clear that the intention
			// is for us to use memory-mapped I/O here, but the C++ STL has no support for it.
			//
			// We do, however, have memory-mapped I/O via the Brawler::MappedFileView class.
			// However, we still need to create a dummy std::ofstream to ensure that both the
			// old file is destroyed and the new file is created.
			{
				std::ofstream psoLibraryFileStream{ psoLibraryFilePath, std::ios::out | std::ios::binary };
			}

			// Re-size the PSO library file to contain our serialized D3D12PipelineLibrary instance.
			const std::size_t serializedPSOSizeBytes = currPipelineLibrary->GetSerializedSize();

			std::filesystem::resize_file(psoLibraryFilePath, static_cast<std::uintmax_t>(serializedPSOSizeBytes), errorCode);
			Util::General::CheckErrorCode(errorCode);

			MappedFileView<FileAccessMode::READ_WRITE> serializedPSOLibraryView{ psoLibraryFilePath, MappedFileView<FileAccessMode::READ_WRITE>::ViewParams{
				.FileOffsetInBytes = 0,
				.ViewSizeInBytes = serializedPSOSizeBytes
			} };
			assert(serializedPSOLibraryView.IsValidView());

			const std::span<std::byte> serializedPSOLibraryDataSpan{ serializedPSOLibraryView.GetMappedData() };
			Util::General::CheckHRESULT(currPipelineLibrary->Serialize(serializedPSOLibraryDataSpan.data(), serializedPSOLibraryDataSpan.size_bytes()));
		}
	}
}
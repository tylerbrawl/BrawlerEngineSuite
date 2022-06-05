module;
#include <atomic>
#include <filesystem>
#include <span>
#include <format>
#include <cassert>
#include <stdexcept>
#include <fstream>
#include "DxDef.h"

module Brawler.D3D12.PSODatabase;
import Util.General;
import Util.Engine;
import Util.Win32;
import Brawler.MappedFileView;
import Brawler.JobSystem;
import Brawler.D3D12.PipelineEnums;
import Brawler.NZStringView;
import Brawler.D3D12.PSODatabase;

namespace
{
	static const std::filesystem::path psoLibraryParentDirectory{ std::filesystem::current_path() / L"Data" };

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
		void PSOLibrary::Initialize(const std::filesystem::path& psoLibraryFileName)
		{
			mPSOLibraryFilePath{ psoLibraryParentDirectory / psoLibraryFileName };
			std::error_code errorCode{};

			const bool fileExists = std::filesystem::exists(mPSOLibraryFilePath, errorCode);
			Util::General::CheckErrorCode(errorCode);

			// Exit early if the PSO library does not exist in the file system.
			if (!fileExists) [[unlikely]]
			{
				mNeedsSerialization.store(true, std::memory_order::relaxed);
				return;
			}

			const bool isDirectory = std::filesystem::is_directory(mPSOLibraryFilePath, errorCode);
			Util::General::CheckErrorCode(errorCode);

			// This shouldn't happen in the common case, but if somehow a directory was created
			// with the same name, then we'll also exit early. We'll be able to delete it later.
			if (isDirectory) [[unlikely]]
			{
				mNeedsSerialization.store(true, std::memory_order::relaxed);
				return;
			}

			const auto fileSize = std::filesystem::file_size(mPSOLibraryFilePath, errorCode);
			Util::General::CheckErrorCode(errorCode);

			mLibraryFileView = MappedFileView<FileAccessMode::READ_ONLY>{ mPSOLibraryFilePath, MappedFileView<FileAccessMode::READ_ONLY>::ViewParams{
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

			// E_INVALIDARG: The cached PSO library data blob is either corrupted or unrecognizable.
			case E_INVALIDARG:
			{
				Util::Win32::WriteFormattedConsoleMessage(L"The PSO cache was somehow corrupted. A full re-compilation of all PSOs is necessary.");

				mNeedsSerialization.store(true, std::memory_order::relaxed);
				return;
			}

			// D3D12_ERROR_DRIVER_VERSION_MISMATCH: The PSO library was created with an older version of either the D3D12 runtime or a graphics driver.
			// D3D12_ERROR_ADAPTER_NOT_FOUND: The PSO library was created for a different video adapter.
			case D3D12_ERROR_DRIVER_VERSION_MISMATCH: [[fallthrough]];
			case D3D12_ERROR_ADAPTER_NOT_FOUND:
			{
				Util::Win32::WriteFormattedConsoleMessage(L"The detected PSO cache was created with a different hardware configuration, or with an older version of the D3D12 runtime. A full re-compilation of all PSOs is necessary.");

				mNeedsSerialization.store(true, std::memory_order::relaxed);
				return;
			}

			// DXGI_ERROR_UNSUPPORTED: Either the OS version or the graphics driver is outdated.
			case DXGI_ERROR_UNSUPPORTED:
			{
				throw std::runtime_error{ "ERROR: The graphics driver of this device is too old to support D3D12 PSO libraries. Please update your graphics drivers before re-launching the application." };

				return;
			}

			default:
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
			Microsoft::WRL::ComPtr<ID3D12PipelineLibrary> oldPipelineLibrary{};
			Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreatePipelineLibrary(nullptr, 0, IID_PPV_ARGS(&oldPipelineLibrary)));

			Microsoft::WRL::ComPtr<Brawler::D3D12PipelineLibrary> currPipelineLibrary{};
			Util::General::CheckHRESULT(oldPipelineLibrary.As(&currPipelineLibrary));

			// The D3D12 PSO library does not support overwriting PSOs. This means that any
			// time we want to make a change to an existing PSO, we have to either bloat the
			// library with unused PSOs or create a new one from scratch. It seems like the
			// latter is the lesser of two evils, especially since the serialization is done
			// asynchronously.

			SerializePSO<static_cast<Brawler::PSOs::PSOID>(0)>(*(currPipelineLibrary.Get()));

			assert(!mPSOLibraryFilePath.empty() && "ERROR: The PSOLibrary was never given a file path to write out the PSO cache to!");

			// DON'T FORGET TO CREATE THE PARENT DIRECTORY!

			std::ofstream psoLibraryFileStream
		}
	}
}
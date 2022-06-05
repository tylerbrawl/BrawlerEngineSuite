module;
#include <filesystem>
#include <atomic>
#include "DxDef.h"

export module Brawler.PSODatabase:PSOLibrary;
import Brawler.MappedFileView;

export namespace Brawler
{
	namespace D3D12
	{
		class PSOLibrary
		{
		public:
			PSOLibrary() = default;

			PSOLibrary(const PSOLibrary& rhs) = delete;
			PSOLibrary& operator=(const PSOLibrary& rhs) = delete;

			PSOLibrary(PSOLibrary&& rhs) noexcept = default;
			PSOLibrary& operator=(PSOLibrary&& rhs) noexcept = default;

			void Initialize(const std::filesystem::path& psoLibraryFileName);

			void SerializePSOLibraryAsync() const;

		private:
			void BeginPSOLibrarySerialization() const;

		private:
			// ID3D12Device1::CreatePipelineLibrary() does not actually copy any data into
			// the created ID3D12PipelineLibrary1 instance, so we need to make sure that the
			// memory which we used to initialize it remains alive for as long as the created
			// ID3D12PipelineLibrary1 instance remains.
			MappedFileView<FileAccessMode::READ_ONLY> mLibraryFileView;

			Microsoft::WRL::ComPtr<Brawler::D3D12PipelineLibrary> mPipelineLibrary;
			std::filesystem::path mPSOLibraryFilePath;
			std::atomic<bool> mNeedsSerialization;
		};
	}
}
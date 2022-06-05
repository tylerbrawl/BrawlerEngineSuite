module;
#include <filesystem>
#include <atomic>
#include <optional>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.PSODatabase:PSOLibrary;
import Brawler.MappedFileView;
import Brawler.D3D12.PipelineEnums;
import Brawler.NZStringView;
import Util.General;

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

			void Initialize();
			void SerializePSOLibraryAsync() const;

			template <Brawler::PSOs::PSOID PSOIdentifier>
			std::optional<Microsoft::WRL::ComPtr<Brawler::D3D12PipelineState>> TryLoadExistingPSO(const D3D12_PIPELINE_STATE_STREAM_DESC& psoDesc);

		private:
			void BeginPSOLibrarySerialization() const;

		private:
			// ID3D12Device1::CreatePipelineLibrary() does not actually copy any data into
			// the created ID3D12PipelineLibrary1 instance, so we need to make sure that the
			// memory which we used to initialize it remains alive for as long as the created
			// ID3D12PipelineLibrary1 instance remains.
			MappedFileView<FileAccessMode::READ_ONLY> mLibraryFileView;

			Microsoft::WRL::ComPtr<Brawler::D3D12PipelineLibrary> mPipelineLibrary;
			std::atomic<bool> mNeedsSerialization;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <Brawler::PSOs::PSOID PSOIdentifier>
		std::optional<Microsoft::WRL::ComPtr<Brawler::D3D12PipelineState>> PSOLibrary::TryLoadExistingPSO(const D3D12_PIPELINE_STATE_STREAM_DESC& psoDesc)
		{
			constexpr Brawler::NZWStringView UNIQUE_PSO_NAME{ Brawler::PSOs::GetUniquePSOName<PSOIdentifier>() };

			if (mPipelineLibrary == nullptr) [[unlikely]]
			{
				// We should already have set mNeedsSerialization to true if no mPipelineLibrary instance
				// was created.
				assert(mNeedsSerialization.load(std::memory_order::relaxed) && "ERROR: No D3D12PipelineLibrary instance was created for the PSOLibrary, but it was never told that it would eventually need serialization! (Did you forget to call PSOLibrary::Initialize()?)");

				return std::optional<Microsoft::WRL::ComPtr<Brawler::D3D12PipelineState>>{};
			}

			Microsoft::WRL::ComPtr<Brawler::D3D12PipelineState> createdPSO{};
			const HRESULT hr = mPipelineLibrary->LoadPipeline(UNIQUE_PSO_NAME.C_Str(), &psoDesc, IID_PPV_ARGS(&createdPSO));

			switch (hr)
			{
			// S_OK: The PSO was created from the cached PSO library successfully.
			case S_OK: [[likely]]
				return std::optional<Microsoft::WRL::ComPtr<Brawler::D3D12PipelineState>>{ std::move(createdPSO) };

			// E_INVALIDARG: A PSO with a matching name and description could not be found in the cached PSO library.
			// In the Brawler Engine, this might happen, for instance, after either creating a new PSODefinition or
			// modifying an existing one.
			case E_INVALIDARG:
			{
				mNeedsSerialization.store(true, std::memory_order::relaxed);

				return std::optional<Microsoft::WRL::ComPtr<Brawler::D3D12PipelineState>>{};
			}

			default: [[unlikely]]
			{
				Util::General::CheckHRESULT(hr);

				assert(false);
				std::unreachable();

				return std::optional<Microsoft::WRL::ComPtr<Brawler::D3D12PipelineState>>{};
			}
			}
		}
	}
}
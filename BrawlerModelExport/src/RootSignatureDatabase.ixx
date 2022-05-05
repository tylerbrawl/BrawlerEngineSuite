module;
#include <array>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.RootSignatureDatabase;
import Brawler.RootSignatures.RootSignatureID;

export namespace Brawler
{
	namespace D3D12
	{
		class RootSignatureDatabase
		{
		public:
			RootSignatureDatabase() = default;

			RootSignatureDatabase(const RootSignatureDatabase& rhs) = delete;
			RootSignatureDatabase& operator=(const RootSignatureDatabase& rhs) = delete;

			RootSignatureDatabase(RootSignatureDatabase&& rhs) noexcept = default;
			RootSignatureDatabase& operator=(RootSignatureDatabase&& rhs) noexcept = default;

			void InitializeDatabase();

			template <Brawler::RootSignatures::RootSignatureID RSIdentifier>
				requires (RSIdentifier != Brawler::RootSignatures::RootSignatureID::COUNT_OR_ERROR)
			Brawler::D3D12RootSignature& GetRootSignature();

			template <Brawler::RootSignatures::RootSignatureID RSIdentifier>
				requires (RSIdentifier != Brawler::RootSignatures::RootSignatureID::COUNT_OR_ERROR)
			const Brawler::D3D12RootSignature& GetRootSignature() const;

		private:
			std::array<Microsoft::WRL::ComPtr<Brawler::D3D12RootSignature>, std::to_underlying(Brawler::RootSignatures::RootSignatureID::COUNT_OR_ERROR)> mRootSigMap;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <Brawler::RootSignatures::RootSignatureID RSIdentifier>
			requires (RSIdentifier != Brawler::RootSignatures::RootSignatureID::COUNT_OR_ERROR)
		Brawler::D3D12RootSignature& RootSignatureDatabase::GetRootSignature()
		{
			assert(mRootSigMap[std::to_underlying(RSIdentifier)] != nullptr && "ERROR: A root signature object was never initialized in the RootSignatureDatabase!");
			return *((mRootSigMap[std::to_underlying(RSIdentifier)]).Get());
		}

		template <Brawler::RootSignatures::RootSignatureID RSIdentifier>
			requires (RSIdentifier != Brawler::RootSignatures::RootSignatureID::COUNT_OR_ERROR)
		const Brawler::D3D12RootSignature& RootSignatureDatabase::GetRootSignature() const
		{
			assert(mRootSigMap[std::to_underlying(RSIdentifier)] != nullptr && "ERROR: A root signature object was never initialized in the RootSignatureDatabase!");
			return *((mRootSigMap[std::to_underlying(RSIdentifier)]).Get());
		}
	}
}
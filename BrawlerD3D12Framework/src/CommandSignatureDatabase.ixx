module;
#include <array>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.CommandSignatureDatabase;
import Brawler.CommandSignatures.CommandSignatureID;

export namespace Brawler
{
	namespace D3D12
	{
		class CommandSignatureDatabase final
		{
		private:
			using CommandSignatureID = CommandSignatures::CommandSignatureID;

		private:
			CommandSignatureDatabase() = default;

		public:
			~CommandSignatureDatabase() = default;

			CommandSignatureDatabase(const CommandSignatureDatabase& rhs) = delete;
			CommandSignatureDatabase& operator=(const CommandSignatureDatabase& rhs) = delete;

			CommandSignatureDatabase(CommandSignatureDatabase&& rhs) noexcept = delete;
			CommandSignatureDatabase& operator=(CommandSignatureDatabase&& rhs) noexcept = delete;

			static CommandSignatureDatabase& GetInstance();

			void InitializeDatabase();

			template <CommandSignatureID CSIdentifier>
				requires (CSIdentifier != CommandSignatureID::COUNT_OR_ERROR)
			Brawler::D3D12CommandSignature& GetCommandSignature();

			template <CommandSignatureID CSIdentifier>
				requires (CSIdentifier != CommandSignatureID::COUNT_OR_ERROR)
			const Brawler::D3D12CommandSignature& GetCommandSignature() const;

		private:
			std::array<Microsoft::WRL::ComPtr<Brawler::D3D12CommandSignature>, std::to_underlying(CommandSignatureID::COUNT_OR_ERROR)> mCSPtrArr;
		};
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <CommandSignatureDatabase::CommandSignatureID CSIdentifier>
			requires (CSIdentifier != CommandSignatureDatabase::CommandSignatureID::COUNT_OR_ERROR)
		Brawler::D3D12CommandSignature& CommandSignatureDatabase::GetCommandSignature()
		{
			assert(mCSPtrArr[std::to_underlying(CSIdentifier)] != nullptr);
			return *((mCSPtrArr[std::to_underlying(CSIdentifier)]).Get());
		}

		template <CommandSignatureDatabase::CommandSignatureID CSIdentifier>
			requires (CSIdentifier != CommandSignatureDatabase::CommandSignatureID::COUNT_OR_ERROR)
		const Brawler::D3D12CommandSignature& CommandSignatureDatabase::GetCommandSignature() const
		{
			assert(mCSPtrArr[std::to_underlying(CSIdentifier)] != nullptr);
			return *((mCSPtrArr[std::to_underlying(CSIdentifier)]).Get());
		}
	}
}
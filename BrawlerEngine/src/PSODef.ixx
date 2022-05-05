module;
#include <functional>
#include <cassert>
#include "DxDef.h"

export module Brawler.IMPL.PSODef;
import Brawler.PipelineEnums;
import Brawler.FilePathHash;

namespace Brawler
{
	namespace IMPL
	{
		enum class PSOStreamType
		{
			VERTEX_SHADER,
			COMPUTE_SHADER,
			MESH_SHADER
		};

		struct BasePSOSubObjectStream
		{
			// Do *NOT* edit these fields within a specialization of
			// PerformPSOStreamRunTimeInitialization()! These are taken care of by the
			// PSOManager upon its initialization.
			
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO CachedPSO;
		};

		template <Brawler::PSOID ID>
		struct PSODefinition
		{
			static_assert(sizeof(ID) != sizeof(ID), "ERROR: An attempt was made to get a PSO description for a given PSOID, but no such description was ever defined!");

			/*
			static constexpr Brawler::RootSignatureID ROOT_SIGNATURE_ID = [ ... ];
			static constexpr Brawler::FilePathHash PSO_NAME_HASH = [ ... ];
			static constexpr std::uint32_t CURRENT_VERSION_NUMBER = [ ... ];

			struct PSOSubObjectStream : public BasePSOSubObjectStream
			{
				[Example]

				CD3DX12_PIPELINE_STATE_STREAM_VS VS;
				CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendDesc;
				CD3DX12_PIPELINE_STATE_STEAM_ROOT_SIGNATURE RootSignature;

				// ...

				[End Example]
			};

			void PerformPSOStreamRunTimeInitialization(PSOSubObjectStream& psoStream)
			{
				[ ... ]
			}
			*/
		};
	}
}

export namespace Brawler
{
	namespace IMPL
	{
		template <Brawler::PSOID ID>
		using PSOSubObjectStream = PSODefinition<ID>::PSOSubObjectStream;

		template <Brawler::PSOID ID>
		constexpr Brawler::RootSignatureID GetPSORootSignatureID()
		{
			return PSODefinition<ID>::ROOT_SIGNATURE_ID;
		}

		template <Brawler::PSOID ID>
		PSOSubObjectStream<ID> GetPSOSubObjectStream()
		{
			constexpr PSOSubObjectStream<ID> psoStream{};
			PSODefinition<ID>::PerformPSOStreamRunTimeInitialization(psoStream);

			assert(psoStream.RootSignature == nullptr && "ERROR: The RootSignature field of a PSO sub-object stream is initialized by the PSOManager!");
			assert(psoStream.CachedPSO.pCachedBlob == nullptr && "ERROR: The CachedPSO field of a PSO sub-object stream is initialized by the PSOManager!");
			
			return psoStream;
		}

		template <Brawler::PSOID ID>
		constexpr Brawler::FilePathHash GetPSONameHash()
		{
			return PSODefinition<ID>::PSO_NAME_HASH;
		}

		template <Brawler::PSOID ID>
		constexpr std::uint32_t GetCurrentPSOVersionNumber()
		{
			return PSODefinition<ID>::CURRENT_VERSION_NUMBER;
		}
	}
}
module;
#include <functional>
#include "DxDef.h"

export module Brawler.D3D12.DirectContext;
import Brawler.D3D12.GPUCommandContext;
import Brawler.D3D12.GPUResourceBinder;
import Brawler.PSOs.PSOID;
import Brawler.PSOs.PSODefinition;
import Util.Engine;
import Brawler.D3D12.RootSignatureDatabase;
import Brawler.D3D12.PSODatabase;

export namespace Brawler
{
	namespace D3D12
	{
		class DirectContext final : public GPUCommandContext<GPUCommandListType::DIRECT>
		{
		public:
			DirectContext() = default;

			DirectContext(const DirectContext& rhs) = delete;
			DirectContext& operator=(const DirectContext& rhs) = delete;

			DirectContext(DirectContext&& rhs) noexcept = default;
			DirectContext& operator=(DirectContext&& rhs) noexcept = default;

			void RecordCommandListIMPL(const std::function<void(DirectContext&)>& recordJob) override;

			template <Brawler::PSOs::PSOID PSOIdentifier>
			GPUResourceBinder<PSOIdentifier> SetPipelineState();
		};
	}
}

// ------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <Brawler::PSOs::PSOID PSOIdentifier>
		GPUResourceBinder<PSOIdentifier> DirectContext::SetPipelineState()
		{
			GetCommandList().SetPipelineState(&(Util::Engine::GetPSODatabase().GetPipelineState<PSOIdentifier>()));

			// Make sure that the root signature is properly set. The D3D12 API states that
			// changing the pipeline state does *NOT* set the root signature; we must do that
			// ourself.
			//
			// In case you are worried about potentially redundantly setting the same root
			// signature, don't be. The D3D12 API guarantees that doing this does *NOT*
			// invalidate any currently set root signature bindings, which is were the bulk of
			// the resource binding cost comes from.
			constexpr Brawler::RootSignatures::RootSignatureID ROOT_SIGNATURE_ID = Brawler::PSOs::GetRootSignature<PSOIdentifier>();

			if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
				GetCommandList().SetGraphicsRootSignature(&(Util::Engine::GetRootSignatureDatabase().GetRootSignature<ROOT_SIGNATURE_ID>()));
			else
				GetCommandList().SetComputeRootSignature(&(Util::Engine::GetRootSignatureDatabase().GetRootSignature<ROOT_SIGNATURE_ID>()));

			return GPUResourceBinder<PSOIdentifier>{ GetCommandList() };
		}
	}
}
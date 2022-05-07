module;
#include <functional>
#include "DxDef.h"

export module Brawler.D3D12.ComputeContext;
import Brawler.D3D12.GPUCommandContext;
import Brawler.D3D12.GPUResourceBinder;
import Util.Engine;
import Brawler.D3D12.PSODatabase;
import Brawler.D3D12.RootSignatureDatabase;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.RootParameterCache;
import Brawler.PSOs.PSOID;
import Brawler.PSOs.PSODefinition;

export namespace Brawler
{
	namespace D3D12
	{
		class ComputeContext final : public GPUCommandContext<GPUCommandQueueType::COMPUTE>
		{
		public:
			ComputeContext() = default;

			ComputeContext(const ComputeContext& rhs) = delete;
			ComputeContext& operator=(const ComputeContext& rhs) = delete;

			ComputeContext(ComputeContext&& rhs) noexcept = default;
			ComputeContext& operator=(ComputeContext&& rhs) noexcept = default;

			void RecordCommandListIMPL(const std::function<void(ComputeContext&)>& recordJob) override;

			template <Brawler::PSOs::PSOID PSOIdentifier>
				requires (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::COMPUTE)
			GPUResourceBinder<PSOIdentifier> SetPipelineState();

			void Dispatch(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY, const std::uint32_t numThreadGroupsZ) const;

		private:
			RootParameterCache mRootParamCache;
		};
	}
}

// -------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <Brawler::PSOs::PSOID PSOIdentifier>
			requires (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::COMPUTE)
		GPUResourceBinder<PSOIdentifier> ComputeContext::SetPipelineState()
		{
			GetCommandList().SetPipelineState(&(PSODatabase::GetInstance().GetPipelineState<PSOIdentifier>()));

			// Make sure that the root signature is properly set. The D3D12 API states that
			// changing the pipeline state does *NOT* set the root signature; we must do that
			// ourself.
			//
			// In case you are worried about potentially redundantly setting the same root
			// signature, don't be. The D3D12 API guarantees that doing this does *NOT*
			// invalidate any currently set root signature bindings, which is where the bulk of
			// the resource binding cost comes from.
			constexpr auto ROOT_SIGNATURE_ID = Brawler::PSOs::GetRootSignature<PSOIdentifier>();
			GetCommandList().SetComputeRootSignature(&(RootSignatureDatabase::GetInstance().GetRootSignature<ROOT_SIGNATURE_ID>()));

			mRootParamCache.SetRootSignature<ROOT_SIGNATURE_ID>();

			return GPUResourceBinder<PSOIdentifier>{ GetCommandList(), mRootParamCache };
		}
	}
}
module;
#include <functional>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.DirectContext;
import Brawler.D3D12.GPUCommandContext;
import Brawler.D3D12.GPUResourceBinder;
import Util.Engine;
import Brawler.D3D12.RootSignatureDatabase;
import Brawler.D3D12.PSODatabase;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.RootParameterCache;
import Brawler.PSOs.PSOID;
import Brawler.PSOs.PSODefinition;

namespace Brawler
{
	namespace D3D12
	{
		template <GPUCommandQueueType QueueType>
		class GPUCommandListRecorder;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class DirectContext final : public GPUCommandContext<GPUCommandQueueType::DIRECT>
		{
		private:
			template <GPUCommandQueueType QueueType>
			friend class GPUCommandListRecorder;

		public:
			DirectContext() = default;

			DirectContext(const DirectContext& rhs) = delete;
			DirectContext& operator=(const DirectContext& rhs) = delete;

			DirectContext(DirectContext&& rhs) noexcept = default;
			DirectContext& operator=(DirectContext&& rhs) noexcept = default;

			void RecordCommandListIMPL(const std::function<void(DirectContext&)>& recordJob) override;

		protected:
			void PrepareCommandListIMPL() override;

		public:
			template <Brawler::PSOs::PSOID PSOIdentifier>
			GPUResourceBinder<PSOIdentifier> SetPipelineState();

			void Dispatch(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY, const std::uint32_t numThreadGroupsZ) const;

		private:
			void PerformSpecialGPUResourceInitialization(I_GPUResource& resource);

		private:
			RootParameterCache mRootParamCache;
			std::optional<Brawler::PSOs::PSOID> mCurrPSOID;
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
			if (!mCurrPSOID.has_value() || *mCurrPSOID != PSOIdentifier)
			{
				GetCommandList().SetPipelineState(&(PSODatabase::GetInstance().GetPipelineState<PSOIdentifier>()));
				mCurrPSOID = PSOIdentifier;

				// Make sure that the root signature is properly set. The D3D12 API states that
				// changing the pipeline state does *NOT* set the root signature; we must do that
				// ourself.
				//
				// In case you are worried about potentially redundantly setting the same root
				// signature, don't be. The D3D12 API guarantees that doing this does *NOT*
				// invalidate any currently set root signature bindings, which is where the bulk of
				// the resource binding cost comes from.
				constexpr auto ROOT_SIGNATURE_ID = Brawler::PSOs::GetRootSignature<PSOIdentifier>();

				if constexpr (Brawler::PSOs::GetPipelineType<PSOIdentifier>() == Brawler::PSOs::PipelineType::GRAPHICS)
					GetCommandList().SetGraphicsRootSignature(&(RootSignatureDatabase::GetInstance().GetRootSignature<ROOT_SIGNATURE_ID>()));
				else
					GetCommandList().SetComputeRootSignature(&(RootSignatureDatabase::GetInstance().GetRootSignature<ROOT_SIGNATURE_ID>()));

				mRootParamCache.SetRootSignature<ROOT_SIGNATURE_ID>();
			}
			
			return GPUResourceBinder<PSOIdentifier>{ GetCommandList(), mRootParamCache };
		}
	}
}
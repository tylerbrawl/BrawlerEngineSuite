module;
#include <functional>
#include "DxDef.h"

export module Brawler.D3D12.DirectContext;
import Brawler.D3D12.GPUCommandContext;
import Brawler.D3D12.GPUResourceBinder;
import Util.Engine;
import Brawler.D3D12.RootSignatureDatabase;
import Brawler.D3D12.PSODatabase;
import Brawler.D3D12.GPUCommandQueueType;
import Brawler.D3D12.PipelineType;
import Brawler.D3D12.RootParameterCache;

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

			template <auto PSOIdentifier>
			GPUResourceBinder<PSOIdentifier> SetPipelineState();

			void Dispatch(const std::uint32_t numThreadGroupsX, const std::uint32_t numThreadGroupsY, const std::uint32_t numThreadGroupsZ) const;

		private:
			void PerformSpecialGPUResourceInitialization(I_GPUResource& resource);

		private:
			RootParameterCache mRootParamCache;
		};
	}
}

// ------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace PSOs
	{
		template <auto PSOIdentifier>
		extern consteval auto GetPipelineType();

		template <auto PSOIdentifier>
		extern consteval auto GetRootSignature();
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <auto PSOIdentifier>
		GPUResourceBinder<PSOIdentifier> DirectContext::SetPipelineState()
		{
			GetCommandList().SetPipelineState(&(PSODatabase<decltype(PSOIdentifier)>::GetInstance().GetPipelineState<PSOIdentifier>()));

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
				GetCommandList().SetGraphicsRootSignature(&(RootSignatureDatabase<decltype(ROOT_SIGNATURE_ID)>::GetInstance().GetRootSignature<ROOT_SIGNATURE_ID>()));
			else
				GetCommandList().SetComputeRootSignature(&(RootSignatureDatabase<decltype(ROOT_SIGNATURE_ID)>::GetInstance().GetRootSignature<ROOT_SIGNATURE_ID>()));

			mRootParamCache.SetRootSignature<ROOT_SIGNATURE_ID>();

			return GPUResourceBinder<PSOIdentifier>{ GetCommandList() };
		}
	}
}
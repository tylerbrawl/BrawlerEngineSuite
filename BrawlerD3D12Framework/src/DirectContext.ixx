module;
#include <functional>
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.DirectContext;
import Brawler.D3D12.GPUCommandContext;
import Brawler.D3D12.ComputeCapableCommandGenerator;
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
		class DirectContext final : public GPUCommandContext<GPUCommandQueueType::DIRECT>, public ComputeCapableCommandGenerator<DirectContext>
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

			/// <summary>
			/// Guarantees that once all of the commands for this frame have been submitted, all of the
			/// presentation callbacks registered with the PresentationManager will be executed. This will
			/// (likely) result in IDXGISwapChain::Present1() being called for every swap chain created by
			/// the application.
			/// 
			/// The original DXGI API for presenting sucks pretty bad, and it seems like Microsoft kind of
			/// just shoehorned this into D3D12. As such, there really is no such thing as a function like
			/// ID3D12GraphicsCommandList::Present(), even though that would make more sense. We believe that
			/// this API is more intuitive. It also naturally allows one to ensure that any back buffers
			/// used for presenting are in the D3D12_RESOURCE_STATE_PRESENT state at the time of presentation
			/// on the GPU timeline by making use of the resource state tracking system already integrated into
			/// the Brawler Engine.
			/// 
			/// Keep in mind that regardless of what RenderPass this command is executed in, the actual
			/// presentation callbacks are executed on the GPU timeline after *ALL* of the commands for the
			/// current frame have been executed. For this reason, it is suggested that this function is called
			/// only in the last RenderPass of a frame. The program may still work fine if any RenderPass
			/// instances have their commands executed after DirectContext::Present() is called in a different
			/// RenderPass if said RenderPasses do not rely on the resources used during presentation, but this
			/// is risky.
			/// </summary>
			void Present() const;

		protected:
			void PrepareCommandListIMPL() override;

		public:
			template <Brawler::PSOs::PSOID PSOIdentifier>
			GPUResourceBinder<PSOIdentifier> SetPipelineState();

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
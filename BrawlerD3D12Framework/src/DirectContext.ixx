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
import Brawler.D3D12.RenderTargetView;

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

			/// <summary>
			/// Clears the I_GPUResource referred to by the RenderTargetView rtv such that the entire view
			/// is filled with a single value. The value chosen to clear the I_GPUResource instance depends
			/// on the std::optional&lt;D3D12_CLEAR_VALUE&gt; returned by I_GPUResource::GetOptimizedClearValue():
			/// 
			///   - If the returned std::optional instance has a value, then this value is used to clear the
			///     resource view.
			/// 
			///   - If the returned std::optional instance has no value, then the entire resource view is
			///     zeroed.
			/// 
			/// Keep in mind that clearing textures with values other than the optimized clear value is often
			/// significantly slower; regardless, if no optimized clear value is provided, then the Brawler Engine
			/// will *NOT* fire an assert. One instance in which not having an optimized clear value specified would
			/// be justifiable is when DirectContext::ClearRenderTargetView() is called for a BufferResource. This
			/// is because buffers cannot have optimized clear values in the D3D12 API.
			/// </summary>
			/// <typeparam name="Format">
			/// - The DXGI_FORMAT of the RenderTargetView. If one is present, the optimized clear value provided
			///   by I_GPUResource::GetOptimizedClearValue() will be interpreted according to this format.
			/// </typeparam>
			/// <typeparam name="ViewDimension">
			/// - The D3D12_RTV_DIMENSION of the RenderTargetView. In other words, this essentially acts as a
			///   compile-time description of the resource's type.
			/// </typeparam>
			/// <param name="rtv">
			/// - The RenderTargetView which describes the contents of the I_GPUResource which are to be cleared.
			///   Refer to the summary for details regarding which value is used to clear the resource.
			/// </param>
			template <DXGI_FORMAT Format, D3D12_RTV_DIMENSION ViewDimension>
			void ClearRenderTargetView(const RenderTargetView<Format, ViewDimension>& rtv) const;

		protected:
			void PrepareCommandListIMPL() override;

		public:
			template <Brawler::PSOs::PSOID PSOIdentifier>
			GPUResourceBinder<PSOIdentifier> SetPipelineState();

		private:
			void ClearRenderTargetViewIMPL(const I_GPUResource& resource, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc) const;
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
		template <DXGI_FORMAT Format, D3D12_RTV_DIMENSION ViewDimension>
		void DirectContext::ClearRenderTargetView(const RenderTargetView<Format, ViewDimension>& rtv) const
		{
			const I_GPUResource& resource{ rtv.GetGPUResource() };
			const D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{ rtv.CreateRTVDescription() };

			ClearRenderTargetViewIMPL(resource, rtvDesc);
		}

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
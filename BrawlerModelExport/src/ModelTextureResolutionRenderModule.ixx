module;
#include <vector>
#include <functional>
#include <mutex>

export module Brawler.ModelTextureResolutionRenderModule;
import Brawler.D3D12.I_RenderModule;
import Brawler.ModelTextureResolutionEventHandle;
import Brawler.D3D12.FrameGraphBuilder;

export namespace Brawler
{
	class ModelTextureResolutionRenderModule final : public D3D12::I_RenderModule
	{
	public:
		ModelTextureResolutionRenderModule() = default;

		ModelTextureResolutionRenderModule(const ModelTextureResolutionRenderModule& rhs) = delete;
		ModelTextureResolutionRenderModule& operator=(const ModelTextureResolutionRenderModule& rhs) = delete;

		ModelTextureResolutionRenderModule(ModelTextureResolutionRenderModule&& rhs) noexcept = delete;
		ModelTextureResolutionRenderModule& operator=(ModelTextureResolutionRenderModule&& rhs) noexcept = delete;

		ModelTextureResolutionEventHandle RegisterTextureResolutionCallback(std::move_only_function<void(D3D12::FrameGraphBuilder&)>&& callback);

	protected:
		void BuildFrameGraph(D3D12::FrameGraphBuilder& builder) override;

	private:
		std::vector<std::move_only_function<void(D3D12::FrameGraphBuilder&)>> mCallbackArr;
		mutable std::mutex mCritSection;
	};
}
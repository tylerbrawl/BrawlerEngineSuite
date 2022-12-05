module;
#include <mutex>

export module Brawler.DeferredRasterGBuffer;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.DepthStencilTexture;
import Brawler.D3D12.RenderTargetBundle;
import Brawler.BlackboardTransientResourceBuilder;
import Brawler.Functional;

export namespace Brawler
{
	class DeferredRasterGBuffer
	{
	public:
		DeferredRasterGBuffer() = default;

		DeferredRasterGBuffer(const DeferredRasterGBuffer& rhs) = delete;
		DeferredRasterGBuffer& operator=(const DeferredRasterGBuffer& rhs) = delete;

		DeferredRasterGBuffer(DeferredRasterGBuffer&& rhs) noexcept = default;
		DeferredRasterGBuffer& operator=(DeferredRasterGBuffer&& rhs) noexcept = default;

		void InitializeTransientResources(BlackboardTransientResourceBuilder& builder);

		D3D12::Texture2D& GetBaseColorRoughnessGBuffer();
		const D3D12::Texture2D& GetBaseColorRoughnessGBuffer() const;

		D3D12::Texture2D& GetEncodedNormalGBuffer();
		const D3D12::Texture2D& GetEncodedNormalGBuffer() const;

		D3D12::Texture2D& GetMetallicGBuffer();
		const D3D12::Texture2D& GetMetallicGBuffer() const;

		D3D12::DepthStencilTexture& GetDepthBuffer();
		const D3D12::DepthStencilTexture& GetDepthBuffer() const;

		template <Brawler::Function<void, D3D12::RenderTargetBundle<3, 1>&> Callback>
		void AccessRenderTargetBundle(const Callback& callback);

		template <Brawler::Function<void, const D3D12::RenderTargetBundle<3, 1>&> Callback>
		void AccessRenderTargetBundle(const Callback& callback) const;

	private:
		D3D12::Texture2D* mBaseColorRoughnessGBufferPtr;
		D3D12::Texture2D* mEncodedNormalGBufferPtr;
		D3D12::Texture2D* mMetallicGBufferPtr;
		D3D12::DepthStencilTexture* mDepthBufferPtr;
		D3D12::RenderTargetBundle<3, 1> mRTBundle;
		mutable std::mutex mRTBundleCritSection;
	};
}

// -----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <Brawler::Function<void, D3D12::RenderTargetBundle<3, 1>&> Callback>
	void DeferredRasterGBuffer::AccessRenderTargetBundle(const Callback& callback)
	{
		const std::scoped_lock<std::mutex> lock{ mRTBundleCritSection };
		callback(mRTBundle);
	}

	template <Brawler::Function<void, const D3D12::RenderTargetBundle<3, 1>&> Callback>
	void DeferredRasterGBuffer::AccessRenderTargetBundle(const Callback& callback) const
	{
		const std::scoped_lock<std::mutex> lock{ mRTBundleCritSection };
		callback(mRTBundle);
	}
}
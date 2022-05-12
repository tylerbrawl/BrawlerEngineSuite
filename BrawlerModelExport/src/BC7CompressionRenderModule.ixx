module;
#include <memory>

export module Brawler.BC7CompressionRenderModule;
import Brawler.D3D12.I_RenderModule;
import Brawler.ThreadSafeVector;
import Brawler.BC7ImageCompressor;

export namespace Brawler
{
	class BC7CompressionRenderModule final : public D3D12::I_RenderModule
	{
	public:
		BC7CompressionRenderModule() = default;

		BC7CompressionRenderModule(const BC7CompressionRenderModule& rhs) = delete;
		BC7CompressionRenderModule& operator=(const BC7CompressionRenderModule& rhs) = delete;

		BC7CompressionRenderModule(BC7CompressionRenderModule&& rhs) noexcept = default;
		BC7CompressionRenderModule& operator=(BC7CompressionRenderModule&& rhs) noexcept = default;

	protected:
		void BuildFrameGraph(D3D12::FrameGraphBuilder& builder) override;

	private:
		ThreadSafeVector<std::unique_ptr<BC7ImageCompressor>> mImageCompressorPtrArr;
	};
}
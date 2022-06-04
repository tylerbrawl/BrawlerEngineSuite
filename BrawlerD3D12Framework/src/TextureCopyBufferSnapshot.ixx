module;
#include "DxDef.h"

export module Brawler.D3D12.TextureCopyBufferSubAllocation:TextureCopyBufferSnapshot;
import Brawler.D3D12.I_BufferSnapshot;

export namespace Brawler
{
	namespace D3D12
	{
		class TextureCopyBufferSubAllocation;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class TextureCopyBufferSnapshot final : public I_BufferSnapshot
		{
		public:
			explicit TextureCopyBufferSnapshot(const TextureCopyBufferSubAllocation& textureCopySubAllocation);

			TextureCopyBufferSnapshot(const TextureCopyBufferSnapshot& rhs) = default;
			TextureCopyBufferSnapshot& operator=(const TextureCopyBufferSnapshot& rhs) = default;

			TextureCopyBufferSnapshot(TextureCopyBufferSnapshot&& rhs) noexcept = default;
			TextureCopyBufferSnapshot& operator=(TextureCopyBufferSnapshot&& rhs) noexcept = default;

			const CD3DX12_TEXTURE_COPY_LOCATION& GetBufferTextureCopyLocation() const;

		private:
			CD3DX12_TEXTURE_COPY_LOCATION mTextureCopyLocation;
		};
	}
}
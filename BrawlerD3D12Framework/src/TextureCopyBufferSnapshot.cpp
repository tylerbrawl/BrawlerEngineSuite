module;
#include "DxDef.h"

module Brawler.D3D12.TextureCopyBufferSubAllocation;

namespace Brawler
{
	namespace D3D12
	{
		TextureCopyBufferSnapshot::TextureCopyBufferSnapshot(const TextureCopyBufferSubAllocation& textureCopySubAllocation) :
			I_BufferSnapshot(textureCopySubAllocation),
			mTextureCopyLocation(textureCopySubAllocation.GetBufferTextureCopyLocation())
		{}

		const CD3DX12_TEXTURE_COPY_LOCATION& TextureCopyBufferSnapshot::GetBufferTextureCopyLocation() const
		{
			return mTextureCopyLocation;
		}
	}
}
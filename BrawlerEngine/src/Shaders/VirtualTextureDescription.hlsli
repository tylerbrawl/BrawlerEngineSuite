namespace BrawlerHLSL
{
	struct VirtualTextureDescription
	{
		uint IndirectionTextureIndex : 24;
		uint Log2VTSize : 8;
		
		uint CombinedPageXCoord : 16;
		uint CombinedPageYCoord : 16;
		
		uint GlobalTextureIndex;
		uint __Pad0;
		
		inline uint GetMipLevelCount()
		{
			return (Log2VTSize + 1);
		}
		
		inline uint GetTextureDimensions()
		{
			return (1 << Log2VTSize);
		}
	};
}
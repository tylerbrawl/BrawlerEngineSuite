namespace BrawlerHLSL
{
	struct DeferredGeometryRasterVSInput
	{
		uint GlobalVertexBufferIndex : GLOBALVBINDEX;  // Input Element Format: DXGI_FORMAT_R32_UINT
		uint PackedModelInstanceDescriptorBufferIndexAndMeshDescriptorBufferIndex : DESCRIPTORBUFFERINDICES;  // Input Element Format: DXGI_FORMAT_R32_UINT
	};
	
	struct DeferredGeometryRasterVSOutput
	{
		float3 PositionCS : SV_Position;
		
		float3 PositionWS : POSITIONWS;
		float3 NormalWS : NORMALWS;
		float3 TangentWS : TANGENTWS;
		float3 BitangentWS : BITANGENTWS;
		float2 UVCoords : UVCOORDS;
		
		nointerpolation uint MaterialDescriptorBufferIndex : MATERIALDESCRIPTORBUFFERINDEX;
	};
	
	struct DeferredGeometryRasterPSInput
	{
		float3 PositionNDCS : SV_Position;
		
		float3 PositionWS : POSITIONWS;
		float3 NormalWS : NORMALWS;
		float3 TangentWS : TANGENTWS;
		float3 BitangentWS : BITANGENTWS;
		float2 UVCoords : UVCOORDS;
		
		nointerpolation uint MaterialDescriptorBufferIndex : MATERIALDESCRIPTORBUFFERINDEX;
	};
	
	struct DeferredGeometryRasterPSOutput
	{
		float4 BaseColorRoughnessGBuffer : SV_Target0;
		float2 EncodedNormalGBuffer : SV_Target1;
		uint MetallicGBuffer : SV_Target2;
	};
	
	struct DeferredGeometryRasterConstantsInfo
	{
		uint ViewID;
	};
}

ConstantBuffer<BrawlerHLSL::DeferredGeometryRasterConstantsInfo> DeferredGeometryRasterConstants : register(b0, space0);
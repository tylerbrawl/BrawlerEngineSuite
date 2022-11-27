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
		/// <summary>
		/// Texture Format: DXGI_FORMAT_R8G8B8A8_UNORM
		///
		/// G-Buffer Layout:
		///   - R8: Base Color Red Channel
		///   - G8: Base Color Green Channel
		///   - B8: Base Color Blue Channel
		///   - A8: GGX Roughness Squared
		/// </summary>
		float4 BaseColorRoughnessGBuffer : SV_Target0;
		
		/// <summary>
		/// Texture Format: DXGI_FORMAT_R8G8_UNORM
		///
		/// G-Buffer Layout:
		///   - R8: Encoded World-Space Surface Normal X
		///   - G8: Encoded World-Space Surface Normal Y
		/// </summary>
		float2 EncodedNormalGBuffer : SV_Target1;
		
		/// <summary>
		/// Texture Format: DXGI_FORMAT_R8_UINT
		///
		/// G-Buffer Layout:
		///   - R8: Metallic Indicator (0 = Dielectric, >0 = Metallic)
		/// </summary>
		uint MetallicGBuffer : SV_Target2;
	};
	
	struct DeferredGeometryRasterConstantsInfo
	{
		uint ViewID;
	};
}

ConstantBuffer<BrawlerHLSL::DeferredGeometryRasterConstantsInfo> DeferredGeometryRasterConstants : register(b0, space0);
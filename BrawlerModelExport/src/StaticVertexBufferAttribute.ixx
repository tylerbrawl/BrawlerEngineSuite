module;
#include <fstream>
#include <vector>
#include <DirectXMath.h>
#include <assimp/scene.h>

export module Brawler.StaticVertexBufferAttribute;
import Brawler.I_MeshAttribute;

export namespace Brawler
{
	class StaticVertexBufferAttribute final : public I_MeshAttribute
	{
	private:
		struct UnpackedStaticVertex
		{
			DirectX::XMFLOAT3 Position;
			DirectX::XMFLOAT3 Normal;
			DirectX::XMFLOAT3 Tangent;
			DirectX::XMFLOAT2 UVCoords;
		};

		struct PackedStaticVertex
		{
			DirectX::XMFLOAT4 PositionAndTangentFrame;
			DirectX::XMFLOAT2 UVCoords;
		};

		struct StaticVertex
		{
			UnpackedStaticVertex UnpackedData;
			PackedStaticVertex PackedData;

			void InitializePackedData();
		};

	public:
		explicit StaticVertexBufferAttribute(const aiScene& scene);

		StaticVertexBufferAttribute(const StaticVertexBufferAttribute& rhs) = delete;
		StaticVertexBufferAttribute& operator=(const StaticVertexBufferAttribute& rhs) = delete;

		StaticVertexBufferAttribute(StaticVertexBufferAttribute&& rhs) noexcept = default;
		StaticVertexBufferAttribute& operator=(StaticVertexBufferAttribute&& rhs) noexcept = default;

		std::vector<std::uint8_t> SerializeAttributeData(const SerializationContext& context) const override;

	private:
		std::vector<StaticVertex> mVertexArr;
	};
}
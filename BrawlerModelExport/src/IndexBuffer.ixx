module;
#include <vector>
#include <assimp/mesh.h>

export module Brawler.IndexBuffer;

export namespace Brawler
{
	class IndexBuffer
	{
	public:
		explicit IndexBuffer(const aiMesh& mesh);

		IndexBuffer(const IndexBuffer& rhs) = delete;
		IndexBuffer& operator=(const IndexBuffer& rhs) = delete;

		IndexBuffer(IndexBuffer&& rhs) noexcept = default;
		IndexBuffer& operator=(IndexBuffer&& rhs) noexcept = default;

	private:
		std::vector<std::uint16_t> mIndices;
	};
}
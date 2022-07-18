module;
#include <vector>

export module Brawler.IndexBuffer;
import Brawler.FilePathHash;
import Brawler.ImportedMesh;
import Brawler.TriangleCluster;
import Brawler.IndexBufferHeader;

export namespace Brawler
{
	class IndexBuffer
	{
	public:
		explicit IndexBuffer(const ImportedMesh& mesh);

		IndexBuffer(const IndexBuffer& rhs) = delete;
		IndexBuffer& operator=(const IndexBuffer& rhs) = delete;

		IndexBuffer(IndexBuffer&& rhs) noexcept = default;
		IndexBuffer& operator=(IndexBuffer&& rhs) noexcept = default;

		void Update();
		bool IsReadyForSerialization() const;

		FilePathHash SerializeIndexBuffer() const;

		IndexBufferHeader GetIndexBufferHeader() const;

	private:
		std::vector<TriangleCluster> mClusterArr;
		const ImportedMesh* mMeshPtr;
	};
}
module;
#include <vector>

export module Brawler.IndexBuffer;
import Brawler.FilePathHash;
import Brawler.ImportedMesh;

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

		std::size_t GetIndexCount() const;

	private:
		std::vector<std::uint32_t> mIndexArr;
		const ImportedMesh* mMeshPtr;
	};
}
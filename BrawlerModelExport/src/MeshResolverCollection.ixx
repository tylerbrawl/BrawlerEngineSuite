module;
#include <vector>
#include <ranges>
#include <span>
#include <memory>
#include <stdexcept>
#include <assimp/mesh.h>

export module Brawler.MeshResolverCollection;
import Brawler.Functional;
import Brawler.ImportedMesh;
import Util.General;
import Brawler.MeshResolverBase;
import Brawler.MeshTypeID;
import Brawler.JobSystem;
import Brawler.ByteStream;

export namespace Brawler
{
	class I_MeshResolverCollection
	{
	protected:
		I_MeshResolverCollection() = default;

	public:
		virtual ~I_MeshResolverCollection() = default;

		I_MeshResolverCollection(const I_MeshResolverCollection& rhs) = delete;
		I_MeshResolverCollection& operator=(const I_MeshResolverCollection& rhs) = delete;

		I_MeshResolverCollection(I_MeshResolverCollection&& rhs) noexcept = default;
		I_MeshResolverCollection& operator=(I_MeshResolverCollection&& rhs) noexcept = default;

		virtual void CreateMeshResolverForImportedMesh(std::unique_ptr<ImportedMesh>&& meshPtr) = 0;

		virtual void Update() = 0;
		virtual bool IsReadyForSerialization() const = 0;

		virtual ByteStream GetSerializedMeshData() const = 0;

		virtual std::size_t GetMeshResolverCount() const = 0;
	};
}

// -------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
	concept IsMeshResolver = std::derived_from<T, MeshResolverBase<T>>;
}

export namespace Brawler
{
	template <typename T>
		requires IsMeshResolver<T>
	class MeshResolverCollection final : public I_MeshResolverCollection
	{
	public:
		MeshResolverCollection() = default;

		MeshResolverCollection(const MeshResolverCollection& rhs) = delete;
		MeshResolverCollection& operator=(const MeshResolverCollection& rhs) = delete;

		MeshResolverCollection(MeshResolverCollection&& rhs) noexcept = default;
		MeshResolverCollection& operator=(MeshResolverCollection&& rhs) noexcept = default;

		void CreateMeshResolverForImportedMesh(std::unique_ptr<ImportedMesh>&& meshPtr) override;

		void Update() override;
		bool IsReadyForSerialization() const override;

		ByteStream GetSerializedMeshData() const override;

		std::size_t GetMeshResolverCount() const override;

	private:
		std::vector<T> mMeshResolverArr;
	};
}

// -------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
		requires IsMeshResolver<T>
	void MeshResolverCollection<T>::CreateMeshResolverForImportedMesh(std::unique_ptr<ImportedMesh>&& meshPtr)
	{
		mMeshResolverArr.emplace_back(std::move(meshPtr));
	}

	template <typename T>
		requires IsMeshResolver<T>
	void MeshResolverCollection<T>::Update()
	{
		Brawler::JobGroup meshResolverUpdateGroup{};
		meshResolverUpdateGroup.Reserve(GetMeshResolverCount());

		for (auto& meshResolver : mMeshResolverArr)
			meshResolverUpdateGroup.AddJob([&meshResolver] () { meshResolver.Update(); });

		meshResolverUpdateGroup.ExecuteJobs();
	}

	template <typename T>
		requires IsMeshResolver<T>
	bool MeshResolverCollection<T>::IsReadyForSerialization() const
	{
		for (const auto& meshResolver : mMeshResolverArr)
		{
			if (!meshResolver.IsReadyForSerialization())
				return false;
		}

		return true;
	}

	template <typename T>
		requires IsMeshResolver<T>
	ByteStream MeshResolverCollection<T>::GetSerializedMeshData() const
	{
		std::vector<ByteStream> serializedMeshDataByteStreamArr{};
		serializedMeshDataByteStreamArr.resize(GetMeshResolverCount());

		Brawler::JobGroup serializeMeshDataGroup{};
		serializeMeshDataGroup.Reserve(GetMeshResolverCount());

		for (const auto i : std::views::iota(0u, GetMeshResolverCount()))
		{
			auto& currByteStream{ serializedMeshDataByteStreamArr[i] };
			auto& currMeshResolver{ mMeshResolverArr[i] };

			serializeMeshDataGroup.AddJob([&currByteStream, &currMeshResolver] ()
			{
				currByteStream << currMeshResolver.SerializeMeshData();
			});
		}

		serializeMeshDataGroup.ExecuteJobs();

		ByteStream meshDataByteStream{};

		for (auto& byteStream : serializedMeshDataByteStreamArr)
			meshDataByteStream << byteStream;

		return meshDataByteStream;
	}

	template <typename T>
		requires IsMeshResolver<T>
	std::size_t MeshResolverCollection<T>::GetMeshResolverCount() const
	{
		return mMeshResolverArr.size();
	}
}
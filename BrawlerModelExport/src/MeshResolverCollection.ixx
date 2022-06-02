module;
#include <vector>
#include <stdexcept>
#include <assimp/mesh.h>

export module Brawler.MeshResolverCollection;
import Brawler.Functional;
import Brawler.ImportedMesh;
import Util.General;
import Brawler.MeshResolverBase;
import Brawler.MeshTypeID;
import Brawler.JobSystem;

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

		virtual void CreateMeshResolverForImportedMesh(ImportedMesh&& mesh) = 0;

		virtual void Update() = 0;
		virtual bool IsReadyForSerialization() const = 0;

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

		void CreateMeshResolverForImportedMesh(ImportedMesh&& mesh) override;

		void Update() override;
		bool IsReadyForSerialization() const override;

		std::size_t GetMeshResolverCount() const override;

	private:
		std::vector<T> mMeshResolverArr;
	};
}

// -------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	MeshTypeID GetMeshTypeID(const aiMesh& mesh)
	{
		// TODO: Add support for skinned meshes. For now, we assert that the mesh is not
		// skinned and simply create a StaticMeshResolver.

		if (mesh.HasBones()) [[unlikely]]
			throw std::runtime_error{ std::string{ "ERROR: The mesh " } + mesh.mName.C_Str() + " is a skinned mesh. Skinned meshes are currently unsupported." };

		return Brawler::MeshTypeID::STATIC;
	}
}

namespace Brawler
{
	template <typename T>
		requires IsMeshResolver<T>
	void MeshResolverCollection<T>::CreateMeshResolverForImportedMesh(ImportedMesh&& mesh)
	{
		mMeshResolverArr.emplace_back(std::move(mesh));
		
		/*
		switch (GetMeshTypeID(mesh.GetMesh()))
		{
		case Brawler::MeshTypeID::STATIC: [[likely]]
		{
			EmplaceMeshResolver<StaticMeshResolver>(std::move(mesh));
			break;
		}

			// TODO: Add support for skinned meshes. For now, we assert that the mesh is not
			// skinned and simply create a StaticMeshResolver.
		case Brawler::MeshTypeID::SKINNED: [[fallthrough]];
		default:
		{
			assert(false && "ERROR: A unique derived MeshResolverBase type was never specified for a given Brawler::MeshTypeID in MeshResolverCollection::CreateMeshResolverForImportedMesh()!");
			std::unreachable();

			break;
		}
		}
		*/
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
	std::size_t MeshResolverCollection<T>::GetMeshResolverCount() const
	{
		return mMeshResolverArr.size();
	}
}
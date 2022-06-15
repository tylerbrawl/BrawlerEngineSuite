module;
#include <memory>
#include <cassert>

export module Brawler.MeshResolverBase;
import Brawler.I_MaterialDefinition;
import Brawler.ImportedMesh;
import Brawler.JobSystem;
import Brawler.SerializedMaterialDefinition;

export namespace Brawler
{
	// As of writing this, the MSVC still doesn't support deducing this for C++20 modules. *sigh*...
	
	template <typename DerivedClass>
	class MeshResolverBase
	{
	public:
#pragma pack(push)
#pragma pack(1)
		struct CompleteSerializedMeshData
		{
			SerializedMaterialDefinition MaterialDefinition;
			typename DerivedClass::SerializedMeshData MeshData;
		};
#pragma pack(pop)

	protected:
		explicit MeshResolverBase(std::unique_ptr<ImportedMesh>&& meshPtr);

	public:
		virtual ~MeshResolverBase() = default;

		MeshResolverBase(const MeshResolverBase& rhs) = delete;
		MeshResolverBase& operator=(const MeshResolverBase& rhs) = delete;

		MeshResolverBase(MeshResolverBase&& rhs) noexcept = default;
		MeshResolverBase& operator=(MeshResolverBase&& rhs) noexcept = default;

		void Update();
		bool IsReadyForSerialization() const;

		CompleteSerializedMeshData SerializeMeshData() const;

	protected:
		const ImportedMesh& GetImportedMesh() const;

	private:
		/// <summary>
		/// Each mesh has a unique material in Assimp, and so the Brawler Engine will (rightly)
		/// reflect that.
		/// </summary>
		std::unique_ptr<I_MaterialDefinition> mMaterialDefinitionPtr;

		std::unique_ptr<ImportedMesh> mImportedMeshPtr;
	};
}

// --------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	std::unique_ptr<Brawler::I_MaterialDefinition> CreateMaterialDefinition(const Brawler::ImportedMesh& mesh);
}

namespace Brawler
{
	template <typename DerivedClass>
	MeshResolverBase<DerivedClass>::MeshResolverBase(std::unique_ptr<ImportedMesh>&& meshPtr) :
		mMaterialDefinitionPtr(CreateMaterialDefinition(*meshPtr)),
		mImportedMeshPtr(std::move(meshPtr))
	{}
	
	template <typename DerivedClass>
	void MeshResolverBase<DerivedClass>::Update()
	{
		Brawler::JobGroup meshResolverUpdateGroup{};
		meshResolverUpdateGroup.Reserve(2);

		meshResolverUpdateGroup.AddJob([this] ()
		{
			mMaterialDefinitionPtr->Update();
		});

		meshResolverUpdateGroup.AddJob([this] ()
		{
			static_cast<DerivedClass*>(this)->UpdateIMPL();
		});

		meshResolverUpdateGroup.ExecuteJobs();
	}

	template <typename DerivedClass>
	bool MeshResolverBase<DerivedClass>::IsReadyForSerialization() const
	{
		return (mMaterialDefinitionPtr->IsReadyForSerialization() && static_cast<const DerivedClass*>(this)->IsReadyForSerializationIMPL());
	}

	template <typename DerivedClass>
	MeshResolverBase<DerivedClass>::CompleteSerializedMeshData MeshResolverBase<DerivedClass>::SerializeMeshData() const
	{
		Brawler::JobGroup meshSerializationGroup{};
		meshSerializationGroup.Reserve(2);

		SerializedMaterialDefinition materialDefinition{};

		meshSerializationGroup.AddJob([this, &materialDefinition] ()
		{
			materialDefinition = mMaterialDefinitionPtr->SerializeMaterial();
		});

		typename DerivedClass::SerializedMeshData partialMeshData{};

		meshSerializationGroup.AddJob([this, &partialMeshData] ()
		{
			partialMeshData = static_cast<const DerivedClass*>(this)->SerializeMeshDataIMPL();
		});

		meshSerializationGroup.ExecuteJobs();

		return CompleteSerializedMeshData{
			.MaterialDefinition{std::move(materialDefinition)},
			.MeshData{std::move(partialMeshData)}
		};
	}

	template <typename DerivedClass>
	const ImportedMesh& MeshResolverBase<DerivedClass>::GetImportedMesh() const
	{
		assert(mImportedMeshPtr.get() != nullptr);
		return *mImportedMeshPtr;
	}
}
module;
#include <memory>
#include <format>
#include <cassert>
#include <assimp/scene.h>

export module Brawler.MeshResolverBase;
import Brawler.I_MaterialDefinition;
import Brawler.ImportedMesh;
import Brawler.OpaqueMaterialDefinition;

export namespace Brawler
{
	// As of writing this, the MSVC still doesn't support decuding this for C++20 modules. *sigh*...
	
	template <typename DerivedClass>
	class MeshResolverBase
	{
	protected:
		explicit MeshResolverBase(ImportedMesh&& mesh);

	public:
		virtual ~MeshResolverBase() = default;

		MeshResolverBase(const MeshResolverBase& rhs) = delete;
		MeshResolverBase& operator=(const MeshResolverBase& rhs) = delete;

		MeshResolverBase(MeshResolverBase&& rhs) noexcept = default;
		MeshResolverBase& operator=(MeshResolverBase&& rhs) noexcept = default;

		void Update();
		bool IsReadyForSerialization() const;

	protected:
		const ImportedMesh& GetImportedMesh() const;

	private:
		/// <summary>
		/// Each mesh has a unique material in Assimp, and so the Brawler Engine will (rightly)
		/// reflect that.
		/// </summary>
		std::unique_ptr<I_MaterialDefinition> mMaterialDefinitionPtr;

		ImportedMesh mImportedMesh;
	};
}

// --------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	std::unique_ptr<Brawler::I_MaterialDefinition> CreateMaterialDefinition(const Brawler::ImportedMesh& mesh)
	{
		const aiMaterial& material{ mesh.GetMeshMaterial() };
		
		aiString materialName{};
		const aiReturn getMaterialNameReturn{ material.Get(AI_MATKEY_NAME, materialName) };
		assert(getMaterialNameReturn == aiReturn::aiReturn_SUCCESS);

		// For now, we only support opaque materials.

		float materialOpacity = 0.0f;
		const aiReturn getMaterialOpacityReturn{ material.Get(AI_MATKEY_OPACITY, materialOpacity) };
		assert(getMaterialOpacityReturn == aiReturn::aiReturn_SUCCESS);

		if (materialOpacity != 1.0f) [[unlikely]]
			throw std::runtime_error{ std::format("ERROR: The material {} is transparent! (Currently, transparent/translucent materials are not supported.)", materialName.C_Str()) };

		return std::make_unique<Brawler::OpaqueMaterialDefinition>(Brawler::ImportedMesh{ mesh });
	}
}

namespace Brawler
{
	template <typename DerivedClass>
	MeshResolverBase<DerivedClass>::MeshResolverBase(ImportedMesh&& mesh) :
		mMaterialDefinitionPtr(CreateMaterialDefinition(mesh)),
		mImportedMesh(std::move(mesh))
	{}
	
	template <typename DerivedClass>
	void MeshResolverBase<DerivedClass>::Update()
	{
		mMaterialDefinitionPtr->Update();
		
		static_cast<DerivedClass*>(this)->UpdateIMPL();
	}

	template <typename DerivedClass>
	bool MeshResolverBase<DerivedClass>::IsReadyForSerialization() const
	{
		return (mMaterialDefinitionPtr->IsReadyForSerialization() && static_cast<const DerivedClass*>(this)->IsReadyForSerializationIMPL());
	}

	template <typename DerivedClass>
	const ImportedMesh& MeshResolverBase<DerivedClass>::GetImportedMesh() const
	{
		return mImportedMesh;
	}
}
module;
#include <memory>

export module Brawler.MeshResolverBase;
import Brawler.I_MaterialDefinition;
import Brawler.ImportedMesh;
import Brawler.ModelTextureBuilderCollection;

export namespace Brawler
{
	// As of writing this, the MSVC still doesn't support deducing this for C++20 modules. *sigh*...
	
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

		ModelTextureBuilderCollection CreateModelTextureBuilders();

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
	std::unique_ptr<Brawler::I_MaterialDefinition> CreateMaterialDefinition(const Brawler::ImportedMesh& mesh);
}

namespace Brawler
{
	template <typename DerivedClass>
	MeshResolverBase<DerivedClass>::MeshResolverBase(ImportedMesh&& mesh) :
		mMaterialDefinitionPtr(CreateMaterialDefinition(mesh)),
		mImportedMesh(std::move(mesh))
	{}

	template <typename DerivedClass>
	ModelTextureBuilderCollection MeshResolverBase<DerivedClass>::CreateModelTextureBuilders()
	{
		return mMaterialDefinitionPtr->CreateModelTextureBuilders();
	}
	
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
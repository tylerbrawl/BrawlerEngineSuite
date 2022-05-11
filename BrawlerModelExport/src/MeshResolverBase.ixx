module;
#include <memory>

export module Brawler.MeshResolverBase;
import Brawler.I_MaterialDefinition;
import Brawler.ImportedMesh;

export namespace Brawler
{
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

		template <typename Self>
		void Update(this Self&& self);

		template <typename Self>
		bool IsReadyForSerialization(this const Self& self);

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
	template <typename Self>
	void MeshResolverBase::Update(this Self&& self)
	{
		mMaterialDefinitionPtr->Update();
		self.UpdateIMPL();
	}

	template <typename Self>
	bool MeshResolverBase::IsReadyForSerialization(this const Self& self)
	{
		return (mMaterialDefinitionPtr->IsReadyForSerialization() && self.IsReadyForSerializationIMPL());
	}
}
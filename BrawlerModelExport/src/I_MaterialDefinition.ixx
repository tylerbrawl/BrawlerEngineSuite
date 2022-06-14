module;

export module Brawler.I_MaterialDefinition;
import Brawler.MaterialID;
import Brawler.ImportedMesh;
import Brawler.SerializedMaterialDefinition;

export namespace Brawler
{
	/// <summary>
	/// This is the class from which all concrete material definition types must derive from.
	/// 
	/// Each class which derives from I_MaterialDefinition represents a different type of material.
	/// For instance, OpaqueMaterialDefinition instances are created when an opaque material is
	/// detected. Each instance will then include unique values for each of that type's required
	/// values. Going back to the previous example, each OpaqueMaterialDefinition would have a
	/// separate diffuse albedo texture.
	/// 
	/// It is okay for multiple derived I_MaterialDefinition instances to end up creating the
	/// exact name material definition. For example, it is not an error if two OpaqueMaterialDefinition
	/// instances have exactly the same set of textures. However, this is unlikely to occur for
	/// static meshes, since Assimp should merge meshes with similar materials in that case.
	/// 
	/// Since each mesh has a unique material, each derived MeshResolverBase instance has a
	/// unique derived I_MaterialDefinition instance. The different derived types of I_MaterialDefinition
	/// define not only the types of textures supported for the material, but also how the definition
	/// is to be serialized.
	/// </summary>
	class I_MaterialDefinition
	{
	protected:
		explicit I_MaterialDefinition(ImportedMesh&& mesh);

	public:
		virtual ~I_MaterialDefinition() = default;

		virtual void Update() = 0;
		virtual bool IsReadyForSerialization() const = 0;

		virtual SerializedMaterialDefinition SerializeMaterial() = 0;

	protected:
		const ImportedMesh& GetImportedMesh() const;

	private:
		ImportedMesh mMesh;
	};
}
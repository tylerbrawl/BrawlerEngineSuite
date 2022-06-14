module;
#include <format>
#include <dxgiformat.h>

module Brawler.OpaqueMaterialDefinition;
import Brawler.ModelTextureSerializer;
import Brawler.ModelTextureID;
import Brawler.MaterialID;
import Brawler.FilePathHash;

namespace Brawler
{
	OpaqueMaterialDefinition::OpaqueMaterialDefinition(ImportedMesh&& mesh) :
		I_MaterialDefinition(std::move(mesh)),
		mDiffuseTextureResolver(GetImportedMesh())
	{}

	void OpaqueMaterialDefinition::Update()
	{
		// If multiple model texture resolvers are being used, it could be a good idea to
		// use a CPU job for each update. However, since we are only using one (for now),
		// we just update that one directly.
		mDiffuseTextureResolver.Update();
	}

	bool OpaqueMaterialDefinition::IsReadyForSerialization() const
	{
		// Serializing the OpaqueMaterialDefinition involves both writing out the
		// FilePathHash for each texture used by the material and serializing the texture
		// data itself. These tasks cannot be completed until the model texture
		// resolvers all report that they are ready for serialization.

		return mDiffuseTextureResolver.IsReadyForSerialization();
	}

	SerializedMaterialDefinition OpaqueMaterialDefinition::SerializeMaterial()
	{
		const ModelTextureSerializer<ModelTextureID::DIFFUSE_ALBEDO> opaqueDiffuseSerializer{ GetImportedMesh(), mDiffuseTextureResolver.GetFinalOpaqueDiffuseTexture() };

		return SerializedMaterialDefinition{
			.Identifier = MaterialID::OPAQUE,
			.DiffuseAlbedoTextureHash = opaqueDiffuseSerializer.GetModelTextureFilePathHash().GetHash()
		};
	}
}
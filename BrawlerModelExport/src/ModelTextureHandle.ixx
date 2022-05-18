module;
#include <assimp/material.h>

export module Brawler.ModelTextureHandle;
import Brawler.ModelTexture;
import Brawler.I_ModelTextureBuilder;
import Brawler.FilePathHash;

export namespace Brawler
{
	/// <summary>
	/// Allowing classes such as those derived from I_MaterialDefinition to gain direct
	/// access to ModelTexture instances is highly dangerous, since ModelTexture instances are
	/// not thread safe, and I_MaterialDefinition instances are commonly updated concurrently.
	/// 
	/// To prevent data races, then, we create a type that offers only a subset of the functions
	/// of ModelTexture instances and allow outside classes to access these, instead. This is
	/// exactly the purpose of the ModelTextureHandle class.
	/// </summary>
	template <aiTextureType TextureType>
	class ModelTextureHandle
	{
	public:
		ModelTextureHandle() = default;
		explicit ModelTextureHandle(const I_ModelTextureBuilder<TextureType>& textureBuilder);

		ModelTextureHandle(const ModelTextureHandle& rhs) = default;
		ModelTextureHandle& operator=(const ModelTextureHandle& rhs) = default;

		ModelTextureHandle(ModelTextureHandle&& rhs) noexcept = default;
		ModelTextureHandle& operator=(ModelTextureHandle&& rhs) noexcept = default;

	private:
		FilePathHash mUniqueTextureNameHash;
	};
}

// ----------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	ModelTextureHandle<TextureType>::ModelTextureHandle(const I_ModelTextureBuilder<TextureType>& textureBuilder) :
		mUniqueTextureNameHash(FilePathHash{textureBuilder.GetUniqueTextureName()})
	{}
}
module;

export module Brawler.I_MaterialDefinition;
import Brawler.MaterialID;

export namespace Brawler
{
	class ModelTextureDatabase;
}

export namespace Brawler
{
	class I_MaterialDefinition
	{
	protected:
		I_MaterialDefinition() = default;

	public:
		virtual ~I_MaterialDefinition() = default;

		virtual void RegisterModelTextures(ModelTextureDatabase& textureDatabase) = 0;

		virtual void Update() = 0;
		virtual bool IsReadyForSerialization() const = 0;

		virtual MaterialID GetMaterialID() const = 0;
	};
}
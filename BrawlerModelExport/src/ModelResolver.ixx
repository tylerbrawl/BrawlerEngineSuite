module;
#include <memory>

export module Brawler.ModelResolver;
import Brawler.MaterialManager;
import Brawler.I_MeshManager;

export namespace Brawler
{
	class ModelResolver
	{
	public:
		ModelResolver() = default;

		ModelResolver(const ModelResolver& rhs) = delete;
		ModelResolver& operator=(const ModelResolver& rhs) = delete;

		ModelResolver(ModelResolver&& rhs) noexcept = default;
		ModelResolver& operator=(ModelResolver&& rhs) noexcept = default;

		/// <summary>
		/// Begins the resolution of the model data as loaded by Assimp. This does all of
		/// the work necessary in order to later serialize this data to our own format.
		/// </summary>
		void ResolveModel();

		/// <summary>
		/// Writes all of the model data out. This can include the mesh data, animations,
		/// and textures, which may all be written out as separate files.
		/// </summary>
		void SerializeModel() const;

	private:
		MaterialManager mMaterialManager;
		std::unique_ptr<I_MeshManager> mMeshManager;
	};
}
module;
#include <assimp/mesh.h>

export module Brawler.I_MeshResolver;

export namespace Brawler
{
	class I_MeshResolver
	{
	protected:
		I_MeshResolver(const aiMesh& mesh);

	public:
		virtual ~I_MeshResolver() = default;

		I_MeshResolver(const I_MeshResolver& rhs) = delete;
		I_MeshResolver& operator=(const I_MeshResolver& rhs) = delete;

		I_MeshResolver(I_MeshResolver&& rhs) noexcept = default;
		I_MeshResolver& operator=(I_MeshResolver&& rhs) noexcept = default;

		virtual void Update() = 0;
		virtual bool IsReadyForSerialization() const = 0;

	private:
		const aiMesh* mMeshPtr;
	};
}
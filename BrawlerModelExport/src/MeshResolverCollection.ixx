module;
#include <vector>
#include <memory>
#include <assimp/mesh.h>

export module Brawler.MeshResolverCollection;
import Brawler.StaticMeshResolver;
import Brawler.Functional;

export namespace Brawler
{
	class MeshResolverCollection
	{
	public:
		MeshResolverCollection() = default;

		MeshResolverCollection(const MeshResolverCollection& rhs) = delete;
		MeshResolverCollection& operator=(const MeshResolverCollection& rhs) = delete;

		MeshResolverCollection(MeshResolverCollection&& rhs) noexcept = default;
		MeshResolverCollection& operator=(MeshResolverCollection&& rhs) noexcept = default;

		void CreateMeshResolverForAIMesh(const aiMesh& mesh);

		void Update();
		bool IsReadyForSerialization() const;

	private:
		/// <summary>
		/// Executes the callback lambda represented by callback on each mesh resolver in this
		/// MeshResolverCollection. Calling this function is preferred over manually looping over
		/// each mesh resolver array in the MeshResolverCollection, since it ensures that no
		/// mesh resolver type is accidentally skipped.
		/// </summary>
		/// <typeparam name="Callback">
		/// - The (anonymous) type of the lambda function represented by callback.
		/// </typeparam>
		/// <param name="callback">
		/// - An instance of a lambda closure which will be executed for each mesh resolver in this
		///   MeshResolverCollection instance. 
		/// 
		///   Specifically, let M represent a mesh resolver. Then, for each mesh resolver in this
		///   MeshResolverCollection instance, this function calls callback(M).
		/// </param>
		template <typename MeshResolverType, Brawler::Function<void, MeshResolverType&> Callback>
		void ForEachMeshResolver(const Callback& callback);

	private:
		std::vector<StaticMeshResolver> mStaticMeshResolverArr;
	};
}

// -------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename MeshResolverType, Brawler::Function<void, MeshResolverType&> Callback>
	void MeshResolverCollection::ForEachMeshResolver(const Callback& callback)
	{
		// Execute the callback for all StaticMeshResolver instances.
		for (auto& staticResolver : mStaticMeshResolverArr)
			callback.operator()<MeshResolverType>(staticResolver);

		// Add additional loops for new MeshResolver types as they are needed.
		// We do this to avoid dynamic polymorphism.
	}
}
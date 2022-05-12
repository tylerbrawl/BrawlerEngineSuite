module;
#include <vector>
#include <memory>
#include <assimp/mesh.h>

export module Brawler.MeshResolverCollection;
import Brawler.StaticMeshResolver;
import Brawler.Functional;
import Brawler.ImportedMesh;

namespace Brawler
{
	using MeshResolverArrayTuple = std::tuple<
		std::vector<StaticMeshResolver>
	>;
}

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

		void CreateMeshResolverForImportedMesh(ImportedMesh&& mesh);

		void Update();
		bool IsReadyForSerialization() const;

		std::size_t GetMeshResolverCount() const;

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
		template <typename Callback>
		void ForEachMeshResolver(const Callback& callback);

		template <typename Callback>
		void ForEachMeshResolver(const Callback& callback) const;

		template <typename MeshResolverType, typename... Args>
		void EmplaceMeshResolver(Args&&... args);

	private:
		MeshResolverArrayTuple mResolverArrTuple;
	};
}

// -------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <std::size_t CurrIndex, typename TupleType, typename Callback>
	void ForEachMeshResolverIMPL(std::add_lvalue_reference_t<TupleType> tuple, const Callback& callback)
	{
		if constexpr (CurrIndex != std::tuple_size_v<MeshResolverArrayTuple>)
		{
			for (auto& meshResolver : std::get<CurrIndex>(tuple))
				callback.operator()<std::remove_reference_t<decltype(meshResolver)>>(meshResolver);

			ForEachMeshResolverIMPL<(CurrIndex + 1), TupleType>(tuple, callback);
		}
	}
}

namespace Brawler
{
	template <typename Callback>
	void MeshResolverCollection::ForEachMeshResolver(const Callback& callback)
	{
		ForEachMeshResolverIMPL<0, MeshResolverArrayTuple>(mResolverArrTuple, callback);
	}

	template <typename Callback>
	void MeshResolverCollection::ForEachMeshResolver(const Callback& callback) const
	{
		ForEachMeshResolverIMPL<0, const MeshResolverArrayTuple>(mResolverArrTuple, callback);
	}

	template <typename MeshResolverType, typename... Args>
	void MeshResolverCollection::EmplaceMeshResolver(Args&&... args)
	{
		std::get<std::vector<MeshResolverType>>(mResolverArrTuple).emplace_back(std::forward<Args>(args)...);
	}
}
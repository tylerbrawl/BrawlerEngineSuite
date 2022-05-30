module;
#include <vector>
#include <memory>
#include <variant>
#include <cassert>
#include <stdexcept>
#include <assimp/mesh.h>

export module Brawler.MeshResolverCollection;
import Brawler.StaticMeshResolver;
import Brawler.Functional;
import Brawler.ImportedMesh;
import Util.General;
import Brawler.ModelTextureBuilderCollection;

namespace Brawler
{
	using RecognizedMeshResolverTuple = std::tuple<
		StaticMeshResolver
	>;

	template <typename T>
	struct MeshResolverArrayTupleSolver
	{};

	template <typename... MeshResolverTypes>
	struct MeshResolverArrayTupleSolver<std::tuple<MeshResolverTypes...>>
	{
		using VariantType = std::variant<std::monostate, std::vector<MeshResolverTypes>...>;
	};
	
	using MeshResolverArrayVariant = typename MeshResolverArrayTupleSolver<RecognizedMeshResolverTuple>::VariantType;
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

		ModelTextureBuilderCollection CreateModelTextureBuilders();

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
		MeshResolverArrayVariant mResolverArrVariant;
	};
}

// -------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename Callback>
	void MeshResolverCollection::ForEachMeshResolver(const Callback& callback)
	{
		std::visit([&callback] (auto& meshResolverArr)
		{
			if constexpr (!std::is_same_v<std::remove_reference_t<decltype(meshResolverArr)>, std::monostate>)
			{
				for (auto& meshResolver : meshResolverArr)
					callback(meshResolver);
			}
		}, mResolverArrVariant);
	}

	template <typename Callback>
	void MeshResolverCollection::ForEachMeshResolver(const Callback& callback) const
	{
		std::visit([&callback] (const auto& meshResolverArr)
		{
			if constexpr (!std::is_same_v<std::remove_cvref_t<decltype(meshResolverArr)>, std::monostate>)
			{
				for (const auto& meshResolver : meshResolverArr)
					callback(meshResolver);
			}
		}, mResolverArrVariant);
	}

	template <typename MeshResolverType, typename... Args>
	void MeshResolverCollection::EmplaceMeshResolver(Args&&... args)
	{
		// The first MeshResolver to be created decides the type of this
		// MeshResolverCollection.
		if (mResolverArrVariant.index() == 0) [[unlikely]]
			mResolverArrVariant = std::vector<MeshResolverType>{};

		std::vector<MeshResolverType>* meshResolverArrPtr = std::get_if<std::vector<MeshResolverType>>(std::addressof(mResolverArrVariant));

		if (meshResolverArrPtr != nullptr) [[likely]]
			meshResolverArrPtr->emplace_back(std::forward<Args>(args)...);
		else [[unlikely]]
			throw std::runtime_error{ "ERROR: An LOD mesh which was considered to be of multiple different mesh formats was detected! (All meshes within an LOD mesh must be of the same mesh format, but different LOD meshes can have different mesh formats.)" };
	}
}
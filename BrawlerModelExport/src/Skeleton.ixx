module;
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <assimp/scene.h>

export module Brawler.Skeleton;
import Brawler.SkeletonBone;
import Brawler.Math.Matrix;

export namespace Brawler
{
	class Skeleton
	{
	private:
		struct SceneGraphTraversalContext
		{
			const std::unordered_set<std::string_view>& BoneNameSet;
			const aiNode& CurrNode;
			SkeletonBone* CurrParentBonePtr;
			const Math::Matrix4x4& ToParentBoneTransform;
		};

	public:
		Skeleton() = default;

		Skeleton(const Skeleton& rhs) = delete;
		Skeleton& operator=(const Skeleton& rhs) = delete;

		Skeleton(Skeleton&& rhs) noexcept = default;
		Skeleton& operator=(Skeleton&& rhs) noexcept = default;

		/// <summary>
		/// Constructs the skeleton hierarchy by traversing the currently loaded
		/// Assimp scene graph, finding all nodes which correspond to bones within
		/// various meshes, and creating a new tree of bone nodes.
		/// </summary>
		void InitializeFromScene();

		SkeletonBone& GetSkeletonBone(const std::string_view boneName);
		const SkeletonBone& GetSkeletonBone(const std::string_view boneName) const;

	private:
		void CreateSkeletonHierarchyFromSceneGraph(const SceneGraphTraversalContext& context);

	private:
		std::unique_ptr<SkeletonBone> mRootBone;

		/// <summary>
		/// This is a map between the name of the aiNode representing a bone and
		/// its corresponding SkeletonBone.
		/// </summary>
		std::unordered_map<std::string_view, SkeletonBone*> mBoneNameMap;
	};
}
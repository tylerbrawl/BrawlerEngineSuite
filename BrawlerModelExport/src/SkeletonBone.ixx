module;
#include <memory>
#include <vector>
#include <assimp/scene.h>

export module Brawler.SkeletonBone;
import Brawler.Math.Matrix;

export namespace Brawler
{
	class Skeleton;
}

export namespace Brawler
{
	class SkeletonBone
	{
	private:
		friend class Skeleton;

	public:
		SkeletonBone(const aiNode& boneNode, const Math::Matrix4x4& toParentBoneMatrix);

		SkeletonBone(const SkeletonBone& rhs) = delete;
		SkeletonBone& operator=(const SkeletonBone& rhs) = delete;

		SkeletonBone(SkeletonBone&& rhs) noexcept = default;
		SkeletonBone& operator=(SkeletonBone&& rhs) noexcept = default;

		const aiNode& GetAiNode() const;

	private:
		SkeletonBone* AddChildBone(const aiNode& childNode, const Math::Matrix4x4& toParentBoneMatrix);

	private:
		/// <summary>
		/// A std::string_view to the name of the bone. The string viewed by this
		/// member is owned by the aiScene, which means that it remains alive for the
		/// duration of the program.
		/// </summary>
		std::string_view mBoneName;

		/// <summary>
		/// This is the matrix which transforms points/vectors from the coordinate
		/// system of this bone to the coordinate system of its parent bone. It is
		/// *NOT* equivalent to the mTransformation member of the corresponding
		/// aiNode instance, nor is it equivalent to the transpose of this matrix!
		/// 
		/// For the root bone, which does not have a parent bone, this matrix
		/// transforms points/vectors from the coordinate system of the bone to the
		/// coordinate system of the mesh.
		/// </summary>
		Math::Matrix4x4 mToParentMatrix;

		/// <summary>
		/// This is the aiNode which corresponds to this bone. For some reason, it
		/// belongs to the same scene graph as everything else which Assimp imports.
		/// </summary>
		const aiNode* mAiNode;

		/// <summary>
		/// This array contains all of the child bones of this SkeletonBone instance.
		/// </summary>
		std::vector<std::unique_ptr<SkeletonBone>> mChildBoneArr;
	};
}
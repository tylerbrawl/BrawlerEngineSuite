module;
#include <cassert>
#include <memory>
#include <vector>
#include <assimp/scene.h>

module Brawler.SkeletonBone;

namespace Brawler
{
	SkeletonBone::SkeletonBone(const aiNode& boneNode, const Math::Matrix4x4& toParentBoneMatrix) :

		// Yes, we are creating a std::string_view for a raw character array. See the comments
		// for this member to understand why this is safe.
		mBoneName(boneNode.mName.C_Str()),

		mToParentMatrix(toParentBoneMatrix),
		mAiNode(&boneNode),
		mChildBoneArr()
	{}

	const aiNode& SkeletonBone::GetAiNode() const
	{
		return *mAiNode;
	}

	SkeletonBone* SkeletonBone::AddChildBone(const aiNode& childNode, const Math::Matrix4x4& toParentBoneMatrix)
	{
		std::unique_ptr<SkeletonBone> childBone{ std::make_unique<SkeletonBone>(childNode, toParentBoneMatrix) };
		SkeletonBone* const childBonePtr = childBone.get();
		
		mChildBoneArr.push_back(std::move(childBone));

		return childBonePtr;
	}
}
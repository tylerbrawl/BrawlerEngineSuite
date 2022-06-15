module;
#include <cassert>
#include <memory>
#include <unordered_set>
#include <span>
#include <vector>
#include <string>
#include <stdexcept>
#include <assimp/scene.h>

module Brawler.Skeleton;
import Brawler.JobSystem;
import Util.General;
import Util.ModelExport;

namespace
{
	/// <summary>
	/// Retrieves the name of each bone in the loaded scene. This function can take
	/// a while to complete, so try to cache the result if you think that you will
	/// need it more than once.
	/// </summary>
	/// <returns>
	/// The function returns the name of every node representing a bone in the loaded
	/// aiScene.
	/// </returns>
	std::unordered_set<std::string_view> GetBoneNameSet()
	{
		const aiScene& scene{ Util::ModelExport::GetScene() };

		// For every mesh in the scene, create a std::unordered_set containing the names 
		// of all of the bones relevant to it. (We assume that models are imported such 
		// that there is only one armature: the one which represents the model's skeleton.)
		//
		// There can be a lot of bones in each individual mesh, and we need to create a
		// std::string for each bone in each mesh. This can take a lot of time, so we do 
		// each of these on a separate thread.
		const std::size_t numMeshes = static_cast<std::size_t>(scene.mNumMeshes);

		std::vector<std::unordered_set<std::string_view>> boneNameSetArr{};
		boneNameSetArr.resize(numMeshes);

		Brawler::JobGroup boneNameGroup{};
		boneNameGroup.Reserve(numMeshes);

		const std::span<const aiMesh*> meshSpan{ const_cast<const aiMesh**>(scene.mMeshes), scene.mNumMeshes };

		// We still don't have std::views::zip. >:(

		for (std::size_t i = 0; i < numMeshes; ++i)
		{
			const aiMesh* const meshPtr = meshSpan[i];
			std::unordered_set<std::string_view>& boneNameSet{ boneNameSetArr[i] };

			boneNameGroup.AddJob([meshPtr, &boneNameSet] ()
			{
				const std::span<const aiBone*> boneSpan{ const_cast<const aiBone**>(meshPtr->mBones), meshPtr->mNumBones };

				for (const auto& bonePtr : boneSpan)
					boneNameSet.insert(std::string_view{ bonePtr->mName.C_Str() });
			});
		}

		boneNameGroup.ExecuteJobs();

		// Get the union of all of the sets. This gives us the name of each bone in the
		// scene. (Rather than iterating through every set, we start with the last set
		// in the boneNameSetArr by std::moving it into the union set. This is more
		// performant.)
		std::unordered_set<std::string_view> boneNameUnionSet{ std::move(boneNameSetArr.back()) };
		boneNameSetArr.pop_back();

		for (const auto& boneNameSet : boneNameSetArr)
			boneNameUnionSet.insert(boneNameSet.begin(), boneNameSet.end());

		return boneNameUnionSet;
	}
}

namespace Brawler
{
	void Skeleton::InitializeFromScene()
	{
		assert(mRootBone == nullptr && "ERROR: An attempt was made to initialize a Brawler::Skeleton twice!");
		
		const std::unordered_set<std::string_view> boneNameSet{ GetBoneNameSet() };
		const aiNode& rootSceneNode{ *(Util::ModelExport::GetScene().mRootNode) };

		const std::optional<Math::Matrix4x4> worldToLocalTransform{ Math::Matrix4x4{ rootSceneNode.mTransformation }.Inverse() };
		assert(worldToLocalTransform.has_value());

		CreateSkeletonHierarchyFromSceneGraph(SceneGraphTraversalContext{
			.BoneNameSet{ boneNameSet },
			.CurrNode{ rootSceneNode },
			.CurrParentBonePtr{ mRootBone.get() },
			.ToParentBoneTransform{ *worldToLocalTransform }
		});
	}

	SkeletonBone& Skeleton::GetSkeletonBone(const std::string_view boneName)
	{
		assert(mBoneNameMap.contains(boneName) && "ERROR: An attempt was made to get a SkeletonBone with a given name, but said bone does not exist!");
		return *(mBoneNameMap.at(boneName));
	}

	const SkeletonBone& Skeleton::GetSkeletonBone(const std::string_view boneName) const
	{
		assert(mBoneNameMap.contains(boneName) && "ERROR: An attempt was made to get a SkeletonBone with a given name, but said bone does not exist!");
		return *(mBoneNameMap.at(boneName));
	}

	void Skeleton::CreateSkeletonHierarchyFromSceneGraph(const SceneGraphTraversalContext& context)
	{
		Brawler::Math::Matrix4x4 toParentBoneTransform{ context.ToParentBoneTransform };

		toParentBoneTransform = (Math::Matrix4x4{ context.CurrNode.mTransformation } * toParentBoneTransform);
		SkeletonBone* currParentBonePtr = context.CurrParentBonePtr;

		// If the current node is a bone, then we need to add it to the hierarchy
		// and reset the toParentBoneTransform to the identity matrix.
		std::string_view currNodeName{ context.CurrNode.mName.C_Str() };

		if (mRootBone == nullptr)
		{
			// Assimp seems to import skeletons by creating a root skeleton node,
			// under which the actual bones used in animations are placed. This
			// node is not used in any animations, and so it will not appear in the
			// list of bone node names which we found.
			//
			// To work-around this, if we have not found this root skeleton node,
			// then we check all of the children of the current node. If the name of
			// one of these nodes is in the bone name set, then we assume that the
			// current node is the root bone node.

			const std::span<const aiNode*> childNodeSpan{ const_cast<const aiNode**>(context.CurrNode.mChildren), context.CurrNode.mNumChildren };
			for (const auto& childPtr : childNodeSpan)
			{
				if (context.BoneNameSet.contains(std::string_view{ childPtr->mName.C_Str() }))
				{
					// One of the children of this node is a bone used in animations. So,
					// we assume that it is the root node.

					mRootBone = std::make_unique<SkeletonBone>(context.CurrNode, toParentBoneTransform);
					currParentBonePtr = mRootBone.get();

					// Add the newly created SkeletonBone to the node-name-to-bone map,
					// mBoneNameMap.
					mBoneNameMap[std::move(currNodeName)] = currParentBonePtr;

					// Reset the to-parent bone transform to be the identity matrix.
					toParentBoneTransform = Math::Matrix4x4{};

					break;
				}
			}
		}
		else if (context.BoneNameSet.contains(currNodeName))
		{
			// The parent SkeletonBone for subsequent recursive calls of this
			// function will be the one which is created for the current aiNode,
			// which itself is a child of whatever the parent SkeletonBone currently
			// is.
			assert(currParentBonePtr != nullptr);
			currParentBonePtr = currParentBonePtr->AddChildBone(context.CurrNode, toParentBoneTransform);

			// Add the newly created SkeletonBone to the node-name-to-bone map,
			// mBoneNameMap.
			mBoneNameMap[std::move(currNodeName)] = currParentBonePtr;

			// Reset the to-parent bone transform to be the identity matrix.
			toParentBoneTransform = Math::Matrix4x4{};
		}

		// Call the function recursively for all of the child nodes of the current
		// node in the scene graph.
		const std::span<const aiNode*> childNodeSpan{ const_cast<const aiNode**>(context.CurrNode.mChildren), context.CurrNode.mNumChildren };
		for (const auto& childNodePtr : childNodeSpan)
			CreateSkeletonHierarchyFromSceneGraph(SceneGraphTraversalContext{
				.BoneNameSet{context.BoneNameSet},
				.CurrNode{*childNodePtr},
				.CurrParentBonePtr{currParentBonePtr},
				.ToParentBoneTransform{toParentBoneTransform}
			});
	}
}
module;
#include <cassert>
#include <assimp/scene.h>
#include <DirectXMath.h>

module Brawler.AssimpSceneLoader;
import Brawler.TransformComponent;

namespace Util
{
	namespace AssimpLoading
	{
		Brawler::Math::Float4x4 CollapseWorldMatrixForAssimpNode(const aiNode& node)
		{
			// Assimp uses matrices designed for column vectors, so we need to multiply
			// their world matrices in the opposite order compared to the rest of the Brawler
			// Engine. Consider the following SceneGraph:
			//
			// Root -> A -> B -> C
			//
			// Let M_X be the world matrix for the SceneNode named X. Traditionally, we would
			// do the following to transform a point/vector v in homogeneous coordinates:
			//
			// v' = v * M_Root * M_A * M_B * M_C
			//
			// We want to find the matrix W defined as follows:
			//
			// W = M_Root * M_A * M_B * M_C
			// W = transpose(transpose(M_Root * M_A * M_B * M_C))
			// W = transpose(transpose(M_C) * transpose(M_B) * transpose(M_A) * transpose(M_Root))
			//
			// So, to get a world matrix which we can use in the rest of the Brawler Engine,
			// we can multiply the Assimp node transformation matrices in "reverse" order (that
			// is, reversed with respect to row vectors) and then take the transpose of that
			// result. This means starting from node and doing left-to-right matrix multiplications
			// with parent node transformation matrices.
			aiMatrix4x4 columnVectorWorldMatrix{ node.mTransformation };
			const aiNode* currParentNodePtr = node.mParent;

			while (currParentNodePtr != nullptr)
			{
				columnVectorWorldMatrix *= currParentNodePtr->mTransformation;
				currParentNodePtr = currParentNodePtr->mParent;
			}

			// Return the transpose of columnVectorWorldMatrix.
			return Brawler::Math::Float4x4{
				columnVectorWorldMatrix[0][0], columnVectorWorldMatrix[1][0], columnVectorWorldMatrix[2][0], columnVectorWorldMatrix[3][0],
				columnVectorWorldMatrix[0][1], columnVectorWorldMatrix[1][1], columnVectorWorldMatrix[2][1], columnVectorWorldMatrix[3][1],
				columnVectorWorldMatrix[0][2], columnVectorWorldMatrix[1][2], columnVectorWorldMatrix[2][2], columnVectorWorldMatrix[3][2],
				columnVectorWorldMatrix[0][3], columnVectorWorldMatrix[1][3], columnVectorWorldMatrix[2][3], columnVectorWorldMatrix[3][3]
			};
		}

		void TransformSceneNode(SceneNode& sceneNode, const aiNode& assimpNode)
		{
			const Brawler::Math::Float4x4 worldMatrix{ CollapseWorldMatrixForAssimpNode(assimpNode) };

			DirectX::XMVECTOR loadedScaleVector{};
			DirectX::XMVECTOR loadedRotationQuaternion{};
			DirectX::XMVECTOR loadedTranslationVector{};
			const DirectX::XMMATRIX loadedWorldMatrix{ worldMatrix.GetDirectXMathMatrix() };

			const bool wasDecompositionSuccessful = DirectX::XMMatrixDecompose(
				&loadedScaleVector,
				&loadedRotationQuaternion,
				&loadedTranslationVector,
				loadedWorldMatrix
			);

			assert(wasDecompositionSuccessful && "ERROR: DirectXMath failed to decompose a world matrix taken from Assimp's scene graph!");

			Brawler::TransformComponent* const transformComponentPtr = sceneNode.GetComponent<Brawler::TransformComponent>();
			assert(transformComponentPtr != nullptr);

			{
				DirectX::XMFLOAT3 storedScaleVector{};
				DirectX::XMStoreFloat3(&storedScaleVector, loadedScaleVector);

				transformComponentPtr->SetScale(Brawler::Math::Float3{ storedScaleVector });
			}

			{
				DirectX::XMFLOAT4 storedRotationQuaternion{};
				DirectX::XMStoreFloat4(&storedRotationQuaternion, loadedRotationQuaternion);

				transformComponentPtr->SetRotation(Brawler::Math::Quaternion{ storedRotationQuaternion });
			}

			{
				DirectX::XMFLOAT3 storedTranslationVector{};
				DirectX::XMStoreFloat3(&storedTranslationVector, loadedTranslationVector);

				transformComponentPtr->SetTranslation(Brawler::Math::Float3{ storedTranslationVector });
			}
		}
	}
}
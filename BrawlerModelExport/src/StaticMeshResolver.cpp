module;
#include <vector>
#include <span>
#include <cassert>
#include <assimp/scene.h>

module Brawler.StaticMeshResolver;
import Brawler.JobSystem;
import Brawler.StaticVertexData;
import Brawler.NormalBoundingConeSolver;

namespace
{
	static constexpr std::size_t MAX_TRIANGLES_PER_NORMAL_BOUNDING_CONE = 256;
	static constexpr std::size_t MAX_INDICES_PER_NORMAL_BOUNDING_CONE = (MAX_TRIANGLES_PER_NORMAL_BOUNDING_CONE * 3);
}

namespace Brawler
{
	StaticMeshResolver::StaticMeshResolver(ImportedMesh&& mesh) :
		MeshResolverBase(std::move(mesh)),
		mVertexBuffer(mesh.GetMesh()),
		mIndexBuffer(mesh.GetMesh()),
		mNormalConeArr()
	{}

	void StaticMeshResolver::UpdateIMPL()
	{
		// We actually do not have an IndexBuffer::Update() function, since index buffers
		// are simple enough to implement that we just do everything we need to in the
		// constructor of the IndexBuffer class.
		//
		// On the other hand, packing the VertexBuffer data does, in fact, take a significant
		// amount of CPU time. Thus, that gets postponed until the mesh resolver is updated
		// in order to better benefit from multithreading.

		if(!mNormalConeArr.empty()) [[likely]]
			mVertexBuffer.Update();

		else [[unlikely]]
		{
			// On the first update for this StaticMeshResolver, we also need to calculate the
			// bounding normal cones.
			const std::span<const UnpackedStaticVertex> unpackedVertexSpan{ mVertexBuffer.GetUnpackedVertexSpan() };
			const std::span<const std::uint16_t> indexSpan{ mIndexBuffer.GetIndexSpan() };

			assert(indexSpan.size() % 3 == 0 && "ERROR: Somehow, an IndexBuffer was created which did not contain a triangle list!");

			const std::size_t numTriangles = (indexSpan.size() / 3);
			const std::size_t numNormalBoundingCones = (numTriangles / MAX_TRIANGLES_PER_NORMAL_BOUNDING_CONE + (numTriangles % MAX_TRIANGLES_PER_NORMAL_BOUNDING_CONE != 0 ? 1 : 0));

			if (numNormalBoundingCones == 0) [[unlikely]]
			{
				mVertexBuffer.Update();
				return;
			}

			mNormalConeArr.resize(numNormalBoundingCones);

			Brawler::JobGroup updateJobGroup{};
			updateJobGroup.Reserve(numNormalBoundingCones + 1);

			updateJobGroup.AddJob([this] ()
			{
				mVertexBuffer.Update();
			});

			// Add a CPU job for each normal bounding cone which we need to create.
			std::size_t numBoundingConeJobsAdded = 0;
			std::size_t numIndicesAdded = 0;

			while (numBoundingConeJobsAdded < numNormalBoundingCones)
			{
				const std::size_t numIndicesForThisJob = std::min(indexSpan.size() - numIndicesAdded, MAX_INDICES_PER_NORMAL_BOUNDING_CONE);

				updateJobGroup.AddJob([unpackedVertexSpan, currIndexSpan = indexSpan.subspan(numIndicesAdded, numIndicesForThisJob), &currCone = mNormalConeArr[numBoundingConeJobsAdded]] ()
				{
					NormalBoundingConeSolver<UnpackedStaticVertex> boundingConeSolver{};
					boundingConeSolver.SetVertexSpan(unpackedVertexSpan);

					currCone = boundingConeSolver.CalculateNormalBoundingCone(currIndexSpan);
				});

				numIndicesAdded += numIndicesForThisJob;
				++numBoundingConeJobsAdded;
			}

			updateJobGroup.ExecuteJobs();
		}
	}

	bool StaticMeshResolver::IsReadyForSerializationIMPL() const
	{
		// For now, this function will essentially always return true. However, this might
		// change in the future if packing vertex data is moved onto the GPU.

		return mVertexBuffer.IsReadyForSerialization();
	}
}
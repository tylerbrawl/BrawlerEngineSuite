module;
#include <vector>
#include <memory>
#include <atomic>

export module Brawler.StaticMeshManager;
import Brawler.I_MeshManager;
import Brawler.StaticMeshResolver;
import Brawler.MeshAttributePointerDescriptor;

export namespace Brawler
{
	class StaticMeshManager final : public I_MeshManager
	{
	public:
		StaticMeshManager();

		StaticMeshManager(const StaticMeshManager& rhs) = delete;
		StaticMeshManager& operator=(const StaticMeshManager& rhs) = delete;

		StaticMeshManager(StaticMeshManager&& rhs) noexcept = default;
		StaticMeshManager& operator=(StaticMeshManager&& rhs) noexcept = default;



	private:
		void CreateSerializationJobs();

	private:
		std::vector<std::unique_ptr<StaticMeshResolver>> mMeshArr;
		std::atomic<std::uint32_t> mActiveSerializationsCounter;
	};
}
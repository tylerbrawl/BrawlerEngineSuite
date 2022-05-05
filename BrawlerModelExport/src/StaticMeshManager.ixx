module;
#include <vector>
#include <memory>

export module Brawler.StaticMeshManager;
import Brawler.I_MeshManager;
import Brawler.StaticMesh;

export namespace Brawler
{
	class StaticMeshManager final : public I_MeshManager
	{
	public:
		StaticMeshManager() = default;

		StaticMeshManager(const StaticMeshManager& rhs) = delete;
		StaticMeshManager& operator=(const StaticMeshManager& rhs) = delete;

		StaticMeshManager(StaticMeshManager&& rhs) noexcept = default;
		StaticMeshManager& operator=(StaticMeshManager&& rhs) noexcept = default;

		void BeginInitialization() override;

		MeshTypeID GetMeshTypeID() const override;

	private:
		std::vector<std::unique_ptr<StaticMesh>> mMeshArr;
	};
}
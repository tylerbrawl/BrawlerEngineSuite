module;

export module Brawler.MeshData;

export namespace Brawler
{
	class MeshData
	{
	public:
		MeshData() = default;

		MeshData(const MeshData& rhs) = delete;
		MeshData& operator=(const MeshData& rhs) = delete;

		MeshData(MeshData&& rhs) noexcept = default;
		MeshData& operator=(MeshData&& rhs) noexcept = default;
	};
}
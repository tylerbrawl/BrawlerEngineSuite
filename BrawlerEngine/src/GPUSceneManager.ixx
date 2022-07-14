module;
#include <tuple>

export module Brawler.GPUSceneManager;
import Brawler.GPUSceneBufferMap;
import Brawler.GPUSceneBufferID;
import Brawler.GPUSceneBuffer;

export namespace Brawler
{
	class GPUSceneManager final
	{
	private:
		GPUSceneManager() = default;

	public:
		~GPUSceneManager() = default;

		GPUSceneManager(const GPUSceneManager& rhs) = delete;
		GPUSceneManager& operator=(const GPUSceneManager& rhs) = delete;

		GPUSceneManager(GPUSceneManager&& rhs) noexcept = delete;
		GPUSceneManager& operator=(GPUSceneManager&& rhs) noexcept = delete;

		void Initialize();

		static GPUSceneManager& GetInstance();

		template <GPUSceneBufferID BufferID>
			requires (BufferID != GPUSceneBufferID::COUNT_OR_ERROR)
		auto& GetGPUSceneBufferResource();

		template <GPUSceneBufferID BufferID>
			requires (BufferID != GPUSceneBufferID::COUNT_OR_ERROR)
		const auto& GetGPUSceneBufferResource() const;

	private:
		GPUSceneBufferTuple mBufferTuple;
	};
}

// ---------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <GPUSceneBufferID BufferID>
		requires (BufferID != GPUSceneBufferID::COUNT_OR_ERROR)
	auto& GPUSceneManager::GetGPUSceneBufferResource()
	{
		return std::get<std::to_underlying(BufferID)>(mBufferTuple);
	}

	template <GPUSceneBufferID BufferID>
		requires (BufferID != GPUSceneBufferID::COUNT_OR_ERROR)
	const auto& GPUSceneManager::GetGPUSceneBufferResource() const
	{
		return std::get<std::to_underlying(BufferID)>(mBufferTuple);
	}
}
module;
#include <unordered_map>
#include <cstdint>

export module Brawler.GPUSceneBufferUpdateSubModule:GPUSceneBufferUpdateMonitor;
import Brawler.GPUSceneBufferID;
import Brawler.GPUSceneBufferMap;

export namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	class GPUSceneBufferUpdateMonitor
	{
	private:
		using ElementType = GPUSceneBufferElementType<BufferID>;

	public:
		GPUSceneBufferUpdateMonitor() = default;

		GPUSceneBufferUpdateMonitor(const GPUSceneBufferUpdateMonitor& rhs) = delete;
		GPUSceneBufferUpdateMonitor& operator=(const GPUSceneBufferUpdateMonitor& rhs) = delete;

		GPUSceneBufferUpdateMonitor(GPUSceneBufferUpdateMonitor&& rhs) noexcept = default;
		GPUSceneBufferUpdateMonitor& operator=(GPUSceneBufferUpdateMonitor&& rhs) noexcept = default;

	private:
		
	};
}
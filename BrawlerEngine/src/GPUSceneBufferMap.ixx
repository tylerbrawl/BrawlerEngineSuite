module;
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <tuple>
#include <optional>

export module Brawler.GPUSceneBufferMap;
import Brawler.GPUSceneBufferID;
import Brawler.GPUSceneBuffer;
import Brawler.GPUSceneTypes;
import Brawler.GPUSceneLimits;

namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	struct GPUSceneBufferInfo
	{
		static_assert(sizeof(BufferID) != sizeof(BufferID), "ERROR: An explicit template specialization of Brawler::GPUSceneBufferInfo was never provided for a Brawler::GPUSceneBufferID value! (See GPUSceneBufferMap.ixx.)");
	};

	template <typename ElementType_, std::size_t NumElements>
	struct GPUSceneBufferInfoInstantiation
	{
		using ElementType = ElementType_;
		static constexpr std::size_t ELEMENT_COUNT = NumElements;

		using GPUSceneBufferType = GPUSceneBuffer<ElementType, ELEMENT_COUNT>;
	};

	template <>
	struct GPUSceneBufferInfo<GPUSceneBufferID::GLOBAL_VERTEX_BUFFER> : public GPUSceneBufferInfoInstantiation<PackedStaticVertex, MAX_VERTEX_BUFFER_ELEMENTS>
	{};

	template <>
	struct GPUSceneBufferInfo<GPUSceneBufferID::GLOBAL_INDEX_BUFFER> : public GPUSceneBufferInfoInstantiation<std::uint32_t, MAX_INDEX_BUFFER_ELEMENTS>
	{};

	template <>
	struct GPUSceneBufferInfo<GPUSceneBufferID::MODEL_INSTANCE_TRANSFORM_DATA_BUFFER> : public GPUSceneBufferInfoInstantiation<ModelInstanceTransformData, MAX_MODEL_INSTANCES>
	{};

	template <>
	struct GPUSceneBufferInfo<GPUSceneBufferID::LOD_MESH_DATA_BUFFER> : public GPUSceneBufferInfoInstantiation<LODMeshData, MAX_MODEL_INSTANCES>
	{};

	template <>
	struct GPUSceneBufferInfo<GPUSceneBufferID::VIEW_TRANSFORM_DATA_BUFFER> : public GPUSceneBufferInfoInstantiation<ViewTransformData, MAX_VIEWS>
	{};

	template <>
	struct GPUSceneBufferInfo<GPUSceneBufferID::VIEW_DIMENSIONS_DATA_BUFFER> : public GPUSceneBufferInfoInstantiation<ViewDimensionsData, MAX_VIEWS>
	{};

	template <>
	struct GPUSceneBufferInfo<GPUSceneBufferID::GLOBAL_TRIANGLE_CLUSTER_BUFFER> : public GPUSceneBufferInfoInstantiation<PackedTriangleCluster, MAX_TRIANGLE_CLUSTERS>
	{};

	template <>
	struct GPUSceneBufferInfo<GPUSceneBufferID::LOD_MESH_DATA_INDEX_BUFFER> : public GPUSceneBufferInfoInstantiation<std::uint32_t, MAX_MODEL_INSTANCES>
	{};

	template <>
	struct GPUSceneBufferInfo<GPUSceneBufferID::VIRTUAL_TEXTURE_DESCRIPTION_BUFFER> : public GPUSceneBufferInfoInstantiation<VirtualTextureDescription, MAX_VIRTUAL_TEXTURE_DESCRIPTIONS>
	{};
}

namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	consteval std::size_t CalculateGPUSceneMemoryConsumptionInBytes()
	{
		if constexpr (BufferID == GPUSceneBufferID::COUNT_OR_ERROR)
			return 0;
		else
		{
			const std::size_t currBufferSize = (sizeof(typename GPUSceneBufferInfo<BufferID>::ElementType) * GPUSceneBufferInfo<BufferID>::ELEMENT_COUNT);
			
			constexpr GPUSceneBufferID NEXT_ID = static_cast<GPUSceneBufferID>(std::to_underlying(BufferID) + 1);
			return (currBufferSize + CalculateGPUSceneMemoryConsumptionInBytes<NEXT_ID>());
		}
	}
}

export namespace Brawler
{
	// Use these values to benchmark how much memory is being consumed by GPU scene buffer data. Keep
	// in mind, however, that only GPU_SCENE_BUFFER_DATA_MEMORY_CONSUMPTION_BYTES is guaranteed to be
	// an exact value.
	// 
	// Be sure to adjust the scene limits both at the top of this source file and in GPUSceneLimits.hlsli
	// as needed for your target platform. (Is this what it's like to develop for consoles? I'm just
	// used to the "allocate until you die" mindset of programming with virtual memory support.)
	//
	// (Pro Tip: Hover over the values with your mouse in Visual Studio, and IntelliSense will show
	// the calculated value automatically. This is possible because the calculations are constant
	// expressions.)

	constexpr std::size_t GPU_SCENE_BUFFER_DATA_MEMORY_CONSUMPTION_BYTES = CalculateGPUSceneMemoryConsumptionInBytes<static_cast<GPUSceneBufferID>(0)>();
	constexpr float GPU_SCENE_BUFFER_DATA_MEMORY_CONSUMPTION_KILOBYTES = (GPU_SCENE_BUFFER_DATA_MEMORY_CONSUMPTION_BYTES / 1024.0f);
	constexpr float GPU_SCENE_BUFFER_DATA_MEMORY_CONSUMPTION_MEGABYTES = (GPU_SCENE_BUFFER_DATA_MEMORY_CONSUMPTION_KILOBYTES / 1024.0f);
	constexpr float GPU_SCENE_BUFFER_DATA_MEMORY_CONSUMPTION_GIGABYTES = (GPU_SCENE_BUFFER_DATA_MEMORY_CONSUMPTION_MEGABYTES / 1024.0f);
}

export namespace Brawler
{
	template <GPUSceneBufferID BufferID>
	using GPUSceneBufferType = typename GPUSceneBufferInfo<BufferID>::GPUSceneBufferType;

	template <GPUSceneBufferID BufferID>
	using GPUSceneBufferElementType = typename GPUSceneBufferInfo<BufferID>::ElementType;

	template <GPUSceneBufferID BufferID>
	consteval std::size_t GetGPUSceneBufferElementCount()
	{
		return GPUSceneBufferInfo<BufferID>::ELEMENT_COUNT;
	}
}

namespace Brawler
{
	template <GPUSceneBufferID BufferID, typename... BufferTypes>
	struct TupleSolver
	{
		using TupleType = typename TupleSolver<static_cast<GPUSceneBufferID>(std::to_underlying(BufferID) - 1), GPUSceneBufferType<BufferID>, BufferTypes...>::TupleType;
	};

	template <typename... BufferTypes>
	struct TupleSolver<static_cast<GPUSceneBufferID>(0), BufferTypes...>
	{
		using TupleType = std::tuple<GPUSceneBufferType<static_cast<GPUSceneBufferID>(0)>, BufferTypes...>;
	};

	template <>
	struct TupleSolver<GPUSceneBufferID::COUNT_OR_ERROR>
	{
		using TupleType = typename TupleSolver<static_cast<GPUSceneBufferID>(std::to_underlying(GPUSceneBufferID::COUNT_OR_ERROR) - 1)>::TupleType;
	};
}

export namespace Brawler
{
	using GPUSceneBufferTuple = typename TupleSolver<GPUSceneBufferID::COUNT_OR_ERROR>::TupleType;
}
module;
#include <cassert>
#include <cstdint>
#include <span>
#include <utility>

export module Brawler.LightDescriptorUpdater;
import Brawler.LightID;
import Brawler.GPUSceneTypes;
import Brawler.D3D12.AlignedByteAddressBufferSubAllocation;
import Brawler.GPUSceneBufferID;
import Brawler.GPUSceneManager;
import Brawler.D3D12.BufferResource;
import Brawler.GPUSceneBufferUpdateOperation;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdateRenderModule;

export namespace Brawler
{
	template <LightID LightIdentifier>
	class LightDescriptorUpdater
	{
	private:
		using SubAllocationType = D3D12::AlignedByteAddressBufferSubAllocation<sizeof(GPUSceneTypes::PackedLightDescriptor), alignof(GPUSceneTypes::PackedLightDescriptor)>;

	public:
		LightDescriptorUpdater();

		~LightDescriptorUpdater();

		LightDescriptorUpdater(const LightDescriptorUpdater& rhs) = delete;
		LightDescriptorUpdater& operator=(const LightDescriptorUpdater& rhs) = delete;
		
		LightDescriptorUpdater(LightDescriptorUpdater&& rhs) noexcept = default;
		LightDescriptorUpdater& operator=(LightDescriptorUpdater&& rhs) noexcept = default;

		void InitializeGPUSceneBufferData(const std::uint32_t lightBufferIndex) const;

	private:
		GPUSceneTypes::PackedLightDescriptor GetPackedLightDescriptor(const std::uint32_t lightBufferIndex) const;

	private:
		SubAllocationType mDescriptorBufferSubAllocation;
	};
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <LightID LightIdentifier>
	LightDescriptorUpdater<LightIdentifier>::LightDescriptorUpdater() :
		mDescriptorBufferSubAllocation()
	{
		[[maybe_unused]] const bool wasReservationSuccessful = GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::LIGHT_DESCRIPTOR_BUFFER>().AssignReservation(mDescriptorBufferSubAllocation);
		assert(wasReservationSuccessful && "ERROR: An attempt to create a reservation from the PackedLightDescriptor GPU scene buffer failed!");
	}

	template <LightID LightIdentifier>
	LightDescriptorUpdater<LightIdentifier>::~LightDescriptorUpdater()
	{
		// Upon destruction of this LightDescriptorUpdater instance, clear the IsValid field of the
		// LightDescriptor we were previously using.
		static constexpr GPUSceneTypes::PackedLightDescriptor INVALID_PACKED_LIGHT_DESCRIPTOR = 0;

		GPUSceneBufferUpdateOperation<GPUSceneBufferID::LIGHT_DESCRIPTOR_BUFFER> descriptorUpdateOperation{ mDescriptorBufferSubAllocation.GetBufferCopyRegion() };
		descriptorUpdateOperation.SetUpdateSourceData(std::span<const GPUSceneTypes::PackedLightDescriptor>{ &INVALID_PACKED_LIGHT_DESCRIPTOR, 1 });

		Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(descriptorUpdateOperation));
	}

	template <LightID LightIdentifier>
	void LightDescriptorUpdater<LightIdentifier>::InitializeGPUSceneBufferData(const std::uint32_t lightBufferIndex) const
	{
		const GPUSceneTypes::PackedLightDescriptor packedDescriptor = GetPackedLightDescriptor(lightBufferIndex);

		GPUSceneBufferUpdateOperation<GPUSceneBufferID::LIGHT_DESCRIPTOR_BUFFER> descriptorUpdateOperation{ mDescriptorBufferSubAllocation.GetBufferCopyRegion() };
		descriptorUpdateOperation.SetUpdateSourceData(std::span<const GPUSceneTypes::PackedLightDescriptor>{ &packedDescriptor, 1 });

		Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(descriptorUpdateOperation));
	}

	template <LightID LightIdentifier>
	GPUSceneTypes::PackedLightDescriptor LightDescriptorUpdater<LightIdentifier>::GetPackedLightDescriptor(const std::uint32_t lightBufferIndex) const
	{
		// Here, we create a PackedLightDescriptor by packing all of the fields in a LightDescriptor (see 
		// LightDescriptor.hlsli) into a single std::uint32_t. HLSL 2021 does support bit fields to make this
		// process easier, but as far as I can tell, it is unspecified how the DXC shader compiler will expect
		// these fields to be laid out in memory. So, we manually pack the value into a std::uint32_t.

		// Start with the value set to 1 to set the IsValid bit in the LightDescriptor.
		GPUSceneTypes::PackedLightDescriptor packedDescriptor = 1;

		// 7 bits are made available for the TypeID. We can ensure that we have enough space at compile time.
		static constexpr std::uint32_t LIGHT_IDENTIFIER_VALUE = std::to_underlying(LightIdentifier);
		static constexpr std::uint32_t SHIFTED_LIGHT_IDENTIFIER_VALUE = (LIGHT_IDENTIFIER_VALUE << 1);

		static constexpr std::uint32_t HIGHEST_SUPPORTED_LIGHT_IDENTIFIER_VALUE = ((1 << 7) - 1);
		static_assert(LIGHT_IDENTIFIER_VALUE <= HIGHEST_SUPPORTED_LIGHT_IDENTIFIER_VALUE, "ERROR: The maximum number of supported light types has been exceeded! (This limit is imposed by the number of bits dedicated to TypeID in PackedLightDescriptor (see LightDescriptor.hlsli).)");

		packedDescriptor |= SHIFTED_LIGHT_IDENTIFIER_VALUE;

		// 24 bits are made available for the LightBufferIndex. This describes the index within the respective
		// light buffer at which the light's data can be found. For instance, if this LightDescriptor is for a 
		// point light, then LightBufferIndex describes the index within the global PointLight GPU scene
		// buffer.
		static constexpr std::uint32_t HIGHEST_SUPPORTED_LIGHT_BUFFER_INDEX = ((1 << 24) - 1);
		assert(lightBufferIndex <= HIGHEST_SUPPORTED_LIGHT_BUFFER_INDEX && "ERROR: The maximum number of light buffer entries for a given light type has been exceeded! (This limit is imposed by the number of bits dedicated to LightBufferIndex in PackedLightDescriptor (see LightDescriptor.hlsli).)");

		packedDescriptor |= (lightBufferIndex << 8);

		return packedDescriptor;
	}
}
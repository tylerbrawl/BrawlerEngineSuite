module;
#include <vector>
#include "DxDef.h"

module Tests.D3D12Resources;
import Brawler.I_GPUResource;
import Util.Math;
import Util.Engine;
import Brawler.Renderer;

namespace
{
	static constexpr std::uint64_t BUFFER_SIZE = Util::Math::KilobytesToBytes(256);

	class GenericBuffer : public Brawler::I_GPUResource
	{
	public:
		GenericBuffer(const std::uint64_t bufferSize = BUFFER_SIZE) :
			I_GPUResource(),
			mBufferSize(bufferSize)
		{}

		Brawler::D3D12_RESOURCE_DESC GetResourceDescription() const override
		{
			Brawler::D3D12_RESOURCE_DESC resourceDesc{};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = mBufferSize;

			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			return resourceDesc;
		}

	private:
		const std::uint64_t mBufferSize;
	};
}

namespace Tests
{
	namespace D3D12Resources
	{
		void RunD3D12ResourcesTests()
		{
			// Test 1: Ensure that a single default buffer can be allocated.

			std::unique_ptr<GenericBuffer> defaultBuffer{ Util::Engine::GetRenderer().CreateDefaultResource<GenericBuffer>(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) };
			Brawler::D3D12Resource& defaultBufferResource{ defaultBuffer->GetD3D12Resource() };

			// Test 2: Ensure that multiple default buffers can be allocated and destroyed.
			{
				static const std::size_t BUFFER_ARRAY_SIZE = 3;
				std::vector<std::unique_ptr<GenericBuffer>> defaultBufferArr{};
				defaultBufferArr.reserve(BUFFER_ARRAY_SIZE);

				for (std::size_t i = 0; i < BUFFER_ARRAY_SIZE; ++i)
					defaultBufferArr.push_back(Util::Engine::GetRenderer().CreateDefaultResource<GenericBuffer>(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

				// Wait for their initialization.
				for (const auto& buffer : defaultBufferArr)
					buffer->GetD3D12Resource();

				std::uint32_t breakHere = 0;
			}

			// Test 3: Try to re-use de-allocated memory.
			{
				std::unique_ptr<GenericBuffer> memoryReuseBuffer{ Util::Engine::GetRenderer().CreateDefaultResource<GenericBuffer>(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) };
				memoryReuseBuffer->GetD3D12Resource();

				std::uint32_t breakHere = 0;
			}

			// Test 4: Make a strangely-sized (i.e., non-power-of-two) allocation.
			{
				std::unique_ptr<GenericBuffer> strangeBuffer{ Util::Engine::GetRenderer().CreateDefaultResource<GenericBuffer>(
					D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
					Util::Math::KilobytesToBytes(69)
				) };
				strangeBuffer->GetD3D12Resource();

				std::uint32_t breakHere = 0;
			}

			// Test 5: Make a very large allocation.
			{
				std::unique_ptr<GenericBuffer> largeBuffer{ Util::Engine::GetRenderer().CreateDefaultResource<GenericBuffer>(
					D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
					Util::Math::MegabytesToBytes(256)
				) };
				largeBuffer->GetD3D12Resource();

				std::uint32_t breakHere = 0;
			}

			// Test 6: Overload the memory usage. We want to see if resources can be placed into evicted
			// heaps when we run out of GPU memory.
			// 
			// UPDATE: As it turns out, trying to create a placed resource within an evicted heap causes
			// the device to be removed.
			//
			// NOTE: This requires that you set a breakpoint in D3DHeapPool::CreatePlacedResource() to
			// detect a failure!
			/*
			{
				std::vector<std::unique_ptr<GenericBuffer>> trashCan{};

				while (true)
				{
					trashCan.push_back(Util::Engine::GetRenderer().CreateDefaultResource<GenericBuffer>(
						D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
						Util::Math::MegabytesToBytes(256)
					));
				}
			}
			*/

			std::uint32_t breakHere = 0;
		}
	}
}
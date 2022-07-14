module;
#include <tuple>

module Brawler.GPUSceneManager;

namespace Brawler
{
	void GPUSceneManager::Initialize()
	{
		// Call GPUSceneBuffer::Initialize() on each GPU scene buffer in the order in which
		// they are stored in GPUSceneBufferTuple. This order automatically matches that of the
		// order in which they are listed in the GPUSceneBufferID enumeration.
		//
		// This makes it easy to guarantee that GPU scene buffers are initialized in the correct
		// order so that shaders can access them using the expected indices.

		const auto initializeBufferLambda = [this]<GPUSceneBufferID BufferID>(this const auto& self)
		{
			if constexpr (BufferID != GPUSceneBufferID::COUNT_OR_ERROR)
			{
				std::get<std::to_underlying(BufferID)>(mBufferTuple).Initialize();

				constexpr GPUSceneBufferID NEXT_ID = static_cast<GPUSceneBufferID>(std::to_underlying(BufferID) + 1);
				self.operator()<NEXT_ID>();
			}
		};

		initializeBufferLambda.operator()<static_cast<GPUSceneBufferID>(0)>();
	}
	
	GPUSceneManager& GPUSceneManager::GetInstance()
	{
		static GPUSceneManager instance{};
		return instance;
	}
}
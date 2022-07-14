module;
#include <utility>
#include <cassert>

module Brawler.I_GPUSceneBufferUpdateSource;

namespace Brawler
{
	I_GPUSceneBufferUpdateSource::I_GPUSceneBufferUpdateSource(D3D12::BufferCopyRegion&& gpuSceneBufferCopyDest) :
		mCopyDestRegion(std::move(gpuSceneBufferCopyDest))
	{}

	const D3D12::BufferCopyRegion& I_GPUSceneBufferUpdateSource::GetGPUSceneBufferCopyDestination() const
	{
		return mCopyDestRegion;
	}
}
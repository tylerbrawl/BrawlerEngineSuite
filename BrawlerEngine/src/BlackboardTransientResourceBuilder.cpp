module;
#include <memory>
#include <vector>
#include <span>

module Brawler.BlackboardTransientResourceBuilder;

namespace Brawler
{
	std::span<std::unique_ptr<D3D12::I_GPUResource>> BlackboardTransientResourceBuilder::GetTransientResourceSpan()
	{
		return std::span<std::unique_ptr<D3D12::I_GPUResource>>{ mTransientResourcePtrArr };
	}

	std::span<const std::unique_ptr<D3D12::I_GPUResource>> BlackboardTransientResourceBuilder::GetTransientResourceSpan() const
	{
		return std::span<const std::unique_ptr<D3D12::I_GPUResource>>{ mTransientResourcePtrArr };
	}
}
module;
#include <optional>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.RTVDSVHandle;

namespace Brawler
{
	namespace D3D12
	{
		template <std::uint32_t DummyParam>
		class GenericCPUVisibleDescriptor
		{
		public:
			GenericCPUVisibleDescriptor() = default;
			explicit GenericCPUVisibleDescriptor(const CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUHandle);

			GenericCPUVisibleDescriptor(const GenericCPUVisibleDescriptor& rhs) = default;
			GenericCPUVisibleDescriptor& operator=(const GenericCPUVisibleDescriptor& rhs) = default;

			GenericCPUVisibleDescriptor(GenericCPUVisibleDescriptor&& rhs) noexcept = default;
			GenericCPUVisibleDescriptor& operator=(GenericCPUVisibleDescriptor&& rhs) noexcept = default;

			CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const;

		private:
			std::optional<CD3DX12_CPU_DESCRIPTOR_HANDLE> mHCPUHandle;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <std::uint32_t DummyParam>
		GenericCPUVisibleDescriptor<DummyParam>::GenericCPUVisibleDescriptor(const CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUHandle) :
			mHCPUHandle(hCPUHandle)
		{}

		template <std::uint32_t DummyParam>
		CD3DX12_CPU_DESCRIPTOR_HANDLE GenericCPUVisibleDescriptor<DummyParam>::GetCPUDescriptorHandle() const
		{
			assert(mHCPUHandle.has_value() && "ERROR: An attempt was made to get the CPU-visible descriptor handle of an uninitialized RTV or DSV!");
			return *mHCPUHandle;
		}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		using RenderTargetViewHandle = GenericCPUVisibleDescriptor<0>;
		using DepthStencilViewHandle = GenericCPUVisibleDescriptor<1>;
	}
}
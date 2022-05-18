module;
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.BindlessSRVSentinel;

namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceDescriptorHeap;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class BindlessSRVSentinel
		{
		private:
			friend class GPUResourceDescriptorHeap;

		public:
			BindlessSRVSentinel() = default;

		private:
			explicit BindlessSRVSentinel(const std::uint32_t bindlessSRVIndex);

		public:
			~BindlessSRVSentinel();

			BindlessSRVSentinel(const BindlessSRVSentinel& rhs) = delete;
			BindlessSRVSentinel& operator=(const BindlessSRVSentinel& rhs) = delete;

			BindlessSRVSentinel(BindlessSRVSentinel&& rhs) noexcept;
			BindlessSRVSentinel& operator=(BindlessSRVSentinel&& rhs) noexcept;

			void SetSRVDescription(D3D12_SHADER_RESOURCE_VIEW_DESC&& srvDesc);

			std::uint32_t GetBindlessSRVIndex() const;

			void UpdateBindlessSRV(Brawler::D3D12Resource& d3dResource) const;

		private:
			void ReturnBindlessSRVIndex();
			
		private:
			std::optional<std::uint32_t> mBindlessSRVIndex;
			D3D12_SHADER_RESOURCE_VIEW_DESC mSRVDesc;
		};
	}
}
module;
#include <optional>
#include "DxDef.h"

export module Brawler.D3D12.I_GPUResource;

export namespace Brawler
{
	namespace D3D12
	{
		struct GPUResourceInitializationInfo;
		class GPUBarrierGroup;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource
		{
		private:
			friend class GPUBarrierGroup;

		protected:
			explicit I_GPUResource(const GPUResourceInitializationInfo& initInfo);

		public:
			virtual ~I_GPUResource() = default;

			I_GPUResource(const I_GPUResource& rhs) = delete;
			I_GPUResource& operator=(const I_GPUResource& rhs) = delete;

			I_GPUResource(I_GPUResource&& rhs) noexcept = default;
			I_GPUResource& operator=(I_GPUResource&& rhs) noexcept = default;

			virtual std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> CreateCBVDescription() const;
			virtual std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> CreateSRVDescription() const;
			virtual std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> CreateUAVDescription() const;

		protected:
			virtual std::optional<D3D12_CLEAR_VALUE> GetOptimizedClearValue() const;

		public:
			Brawler::D3D12Resource& GetD3D12Resource();
			const Brawler::D3D12Resource& GetD3D12Resource() const;

			const Brawler::D3D12_RESOURCE_DESC& GetResourceDescription() const;

			D3D12_RESOURCE_STATES GetCurrentResourceState() const;

		private:
			void SetCurrentResourceState(const D3D12_RESOURCE_STATES currentState);

		private:
			Microsoft::WRL::ComPtr<D3D12MA::Allocation> mD3D12MAAllocation;
			Microsoft::WRL::ComPtr<Brawler::D3D12Resource> mResource;

			/// <summary>
			/// Rather than calling ID3D12Resource::GetDesc(), we create a cached copy of the
			/// resource description and refer to that. This is largely because GetDesc() is
			/// (for some unholy reason) a non-const member function, but doing it like this
			/// also allows us to retain information about the sampler feedback region, if it
			/// is specified.
			/// </summary>
			Brawler::D3D12_RESOURCE_DESC mResourceDesc;

			D3D12_RESOURCE_STATES mCurrState;
		};
	}
}
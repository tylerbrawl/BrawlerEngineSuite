module;
#include <shared_mutex>
#include "DxDef.h"

export module Brawler.D3D12.D3D12ResourceContainer;

export namespace Brawler
{
	namespace D3D12
	{
		class I_GPUResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class D3D12ResourceContainer
		{
		protected:
			explicit D3D12ResourceContainer(I_GPUResource& owningResource);

		public:
			virtual ~D3D12ResourceContainer() = default;

			D3D12ResourceContainer(const D3D12ResourceContainer& rhs) = delete;
			D3D12ResourceContainer& operator=(const D3D12ResourceContainer& rhs) = delete;

			D3D12ResourceContainer(D3D12ResourceContainer&& rhs) noexcept = default;
			D3D12ResourceContainer& operator=(D3D12ResourceContainer&& rhs) noexcept = default;

			Brawler::D3D12Resource& GetD3D12Resource() const;

		protected:
			I_GPUResource& GetGPUResource();
			const I_GPUResource& GetGPUResource() const;

			void SetD3D12Resource(Microsoft::WRL::ComPtr<Brawler::D3D12Resource>&& d3dResource);

		private:
			I_GPUResource* mOwningResourcePtr;
			Microsoft::WRL::ComPtr<Brawler::D3D12Resource> mD3DResource;
			mutable std::shared_mutex mCritSection;
		};
	}
}
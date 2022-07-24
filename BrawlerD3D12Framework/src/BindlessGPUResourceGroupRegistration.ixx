module;
#include <atomic>

export module Brawler.D3D12.BindlessGPUResourceGroupRegistration;

export namespace Brawler
{
	namespace D3D12
	{
		class BindlessGPUResourceGroupRegistration
		{
		public:
			BindlessGPUResourceGroupRegistration();

			BindlessGPUResourceGroupRegistration(const BindlessGPUResourceGroupRegistration& rhs) = delete;
			BindlessGPUResourceGroupRegistration& operator=(const BindlessGPUResourceGroupRegistration& rhs) = delete;

			BindlessGPUResourceGroupRegistration(BindlessGPUResourceGroupRegistration&& rhs) noexcept = default;
			BindlessGPUResourceGroupRegistration& operator=(BindlessGPUResourceGroupRegistration&& rhs) noexcept = default;

			void MarkAsInvalidated();
			bool IsValid() const;

		private:
			std::atomic<bool> mIsValid;
		};
	}
}
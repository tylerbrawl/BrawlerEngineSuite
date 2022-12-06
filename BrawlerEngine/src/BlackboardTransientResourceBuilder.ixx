module;
#include <concepts>
#include <memory>
#include <vector>
#include <span>

export module Brawler.BlackboardTransientResourceBuilder;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.GPUResourceLifetimeType;

export namespace Brawler 
{
	class BlackboardTransientResourceBuilder
	{
	public:
		BlackboardTransientResourceBuilder() = default;

		BlackboardTransientResourceBuilder(const BlackboardTransientResourceBuilder& rhs) = delete;
		BlackboardTransientResourceBuilder& operator=(const BlackboardTransientResourceBuilder& rhs) = delete;

		BlackboardTransientResourceBuilder(BlackboardTransientResourceBuilder&& rhs) noexcept = default;
		BlackboardTransientResourceBuilder& operator=(BlackboardTransientResourceBuilder&& rhs) noexcept = default;

		template <typename T, typename... Args>
			requires (std::derived_from<T, D3D12::I_GPUResource> && std::constructible_from<T, Args...>)
		T& CreateTransientResource(Args&&... args);

		std::span<std::unique_ptr<D3D12::I_GPUResource>> GetTransientResourceSpan();
		std::span<const std::unique_ptr<D3D12::I_GPUResource>> GetTransientResourceSpan() const;

	private:
		std::vector<std::unique_ptr<D3D12::I_GPUResource>> mTransientResourcePtrArr;
	};
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T, typename... Args>
		requires (std::derived_from<T, D3D12::I_GPUResource> && std::constructible_from<T, Args...>)
	T& BlackboardTransientResourceBuilder::CreateTransientResource(Args&&... args)
	{
		std::unique_ptr<T> transientResourcePtr{ std::make_unique<T>(std::forward<Args>(args)...) };

		// Mark the resource as transient.
		transientResourcePtr->SetGPUResourceLifetimeType(D3D12::GPUResourceLifetimeType::TRANSIENT);

		T* const rawTransientResourcePtr = transientResourcePtr.get();
		mTransientResourcePtrArr.push_back(std::move(transientResourcePtr));

		return *rawTransientResourcePtr;
	}
}
module;
#include <memory>
#include <atomic>

export module Brawler.ModelLoadEventHandle;

export namespace Brawler
{
	class ModelLoadEventHandle
	{
	public:
		ModelLoadEventHandle() = default;

		ModelLoadEventHandle(const ModelLoadEventHandle& rhs) = default;
		ModelLoadEventHandle& operator=(const ModelLoadEventHandle& rhs) = default;

		ModelLoadEventHandle(ModelLoadEventHandle&& rhs) noexcept = default;
		ModelLoadEventHandle& operator=(ModelLoadEventHandle&& rhs) noexcept = default;

	private:
		std::shared_ptr<std::atomic<bool>> mEventSignalled;
	};
}
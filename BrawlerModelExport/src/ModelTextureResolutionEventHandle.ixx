module;
#include <cstdint>

export module Brawler.ModelTextureResolutionEventHandle;

export namespace Brawler
{
	class ModelTextureResolutionEventHandle
	{
	public:
		ModelTextureResolutionEventHandle();

		ModelTextureResolutionEventHandle(const ModelTextureResolutionEventHandle& rhs) = default;
		ModelTextureResolutionEventHandle& operator=(const ModelTextureResolutionEventHandle& rhs) = default;

		ModelTextureResolutionEventHandle(ModelTextureResolutionEventHandle&& rhs) noexcept = default;
		ModelTextureResolutionEventHandle& operator=(ModelTextureResolutionEventHandle&& rhs) noexcept = default;

		bool IsEventComplete() const;

	private:
		std::uint64_t mCreationFrameNum;
	};
}
module;

export module Brawler.DeferredRasterGBuffer;

export namespace Brawler
{
	class DeferredRasterGBuffer
	{
	public:
		DeferredRasterGBuffer() = default;

		DeferredRasterGBuffer(const DeferredRasterGBuffer& rhs) = delete;
		DeferredRasterGBuffer& operator=(const DeferredRasterGBuffer& rhs) = delete;

		DeferredRasterGBuffer(DeferredRasterGBuffer&& rhs) noexcept = default;
		DeferredRasterGBuffer& operator=(DeferredRasterGBuffer&& rhs) noexcept = default;

	private:

	};
}
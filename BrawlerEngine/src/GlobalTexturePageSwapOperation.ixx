module;

export module Brawler.GlobalTexturePageSwapOperation;
import Brawler.OptionalRef;
import Brawler.D3D12.Texture2D;

export namespace Brawler
{
	class GlobalTexturePageSwapOperation
	{
	public:
		GlobalTexturePageSwapOperation() = default;

		GlobalTexturePageSwapOperation(const GlobalTexturePageSwapOperation& rhs) = delete;
		GlobalTexturePageSwapOperation& operator=(const GlobalTexturePageSwapOperation& rhs) = delete;

		GlobalTexturePageSwapOperation(GlobalTexturePageSwapOperation&& rhs) noexcept = default;
		GlobalTexturePageSwapOperation& operator=(GlobalTexturePageSwapOperation&& rhs) noexcept = default;

	private:
		D3D12::Texture2D* mGlobalTexturePtr;
	};
}
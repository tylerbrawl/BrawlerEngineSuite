module;
#include <optional>

export module Brawler.GlobalTexturePageSwapOperation;
import Brawler.D3D12.Texture2D;
import Brawler.VirtualTextureLogicalPage;
import Brawler.GlobalTextureReservedPage;

export namespace Brawler
{
	class GlobalTexturePageSwapOperation
	{
	public:
		// NOTE: We copy the GlobalTextureReservedPage instances by value because we do not want
		// to end up with different data if something changes after the GlobalTexturePageSwapOperation
		// instance is created.
		struct InitInfo
		{
			D3D12::Texture2D& GlobalTexture;
			GlobalTextureReservedPage NewReservedPage;
			std::optional<GlobalTextureReservedPage> OldReservedPage;
			std::uint16_t GlobalTextureXPageCoordinates;
			std::uint16_t GlobalTextureYPageCoordinates;
		};

	public:
		explicit GlobalTexturePageSwapOperation(InitInfo&& initInfo);

		GlobalTexturePageSwapOperation(const GlobalTexturePageSwapOperation& rhs) = delete;
		GlobalTexturePageSwapOperation& operator=(const GlobalTexturePageSwapOperation& rhs) = delete;

		GlobalTexturePageSwapOperation(GlobalTexturePageSwapOperation&& rhs) noexcept = default;
		GlobalTexturePageSwapOperation& operator=(GlobalTexturePageSwapOperation&& rhs) noexcept = default;

		D3D12::Texture2D& GetGlobalTexture() const;
		const GlobalTextureReservedPage& GetReplacementPage() const;

		bool IsReplacingOlderPage() const;
		const GlobalTextureReservedPage& GetPreviousPage() const;

		std::uint16_t GetGlobalTextureXPageCoordinates() const;
		std::uint16_t GetGlobalTextureYPageCoordinates() const;

	private:
		InitInfo mInitInfo;
	};
}
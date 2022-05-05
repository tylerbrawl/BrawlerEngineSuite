module;
#include <unordered_map>
#include "DxDef.h"

export module Brawler.DisplayAdapter;
import Brawler.CommandListType;
import Brawler.CommandQueue;

export namespace Brawler
{
	class DisplayAdapter
	{
	public:
		enum class Vendor
		{
			NVIDIA,
			AMD,
			INTEL,
			UNKNOWN
		};

	public:
		explicit DisplayAdapter(Microsoft::WRL::ComPtr<Brawler::DXGIAdapter>&& dxgiAdapter);

		void Initialize();

		// Returns the identifier of the independent hardware vendor (IHV) of
		// the display adapter represented by the DisplayAdapter instance. This
		// is useful for creating separate render paths for DirectX 12 performance.
		Vendor GetVendor() const;

		// Returns the total VRAM size of the display adapter.
		std::size_t GetTotalVRAMSize() const;

		DXGIAdapter& GetAdapter();
		const DXGIAdapter& GetAdapter() const;
		Brawler::D3D12Device& GetD3D12Device();
		const Brawler::D3D12Device& GetD3D12Device() const;

		Brawler::CommandQueue& GetCommandQueue(const Brawler::CommandListType cmdListType);
		const Brawler::CommandQueue& GetCommandQueue(const Brawler::CommandListType cmdListType) const;

		/// <summary>
		/// This function checks to make sure that the display adapter supports all of
		/// the features required by the Brawler Engine to function.
		/// </summary>
		/// <returns>
		/// The function returns true if the adapter is compatible with the Brawler
		/// Engine and false otherwise.
		/// </returns>
		bool IsSupportedAdapter() const;

	private:
		void InitializeAdapterInformation();
		void CreateD3D12Device();
		void InitializeCommandQueues();

	private:
		Microsoft::WRL::ComPtr<Brawler::DXGIAdapter> mAdapter;
		Microsoft::WRL::ComPtr<Brawler::D3D12Device> mD3DDevice;
		Vendor mVendor;
		std::size_t mVRAMSize;
		std::unordered_map<Brawler::CommandListType, Brawler::CommandQueue> mCmdQueueMap;
	};

	DisplayAdapter& GetDisplayAdapter();
}
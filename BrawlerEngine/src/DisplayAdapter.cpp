module;
#include <unordered_map>
#include <cassert>
#include "DxDef.h"

module Brawler.DisplayAdapter;
import Brawler.Application;
import Brawler.Renderer;
import Brawler.CommandListType;
import Brawler.CommandQueue;
import Util.General;

namespace
{
	// This is the minimum D3D feature level which is required by the Brawler Engine.
	static constexpr D3D_FEATURE_LEVEL MINIMUM_FEATURE_LEVEL = D3D_FEATURE_LEVEL_11_0;
}

namespace Brawler
{
	DisplayAdapter::DisplayAdapter(Microsoft::WRL::ComPtr<Brawler::DXGIAdapter>&& dxgiAdapter) :
		mAdapter(std::move(dxgiAdapter)),
		mD3DDevice(nullptr),
		mVendor(Vendor::UNKNOWN),
		mVRAMSize(0),
		mCmdQueueMap()
	{
		if (mAdapter != nullptr)
		{
			InitializeAdapterInformation();
			CreateD3D12Device();
		}
	}

	void DisplayAdapter::Initialize()
	{
		InitializeCommandQueues();
	}

	DisplayAdapter::Vendor DisplayAdapter::GetVendor() const
	{
		return mVendor;
	}

	std::size_t DisplayAdapter::GetTotalVRAMSize() const
	{
		return mVRAMSize;
	}

	DXGIAdapter& DisplayAdapter::GetAdapter()
	{
		return *(mAdapter.Get());
	}

	const DXGIAdapter& DisplayAdapter::GetAdapter() const
	{
		return *(mAdapter.Get());
	}

	Brawler::D3D12Device& DisplayAdapter::GetD3D12Device()
	{
		return *(mD3DDevice.Get());
	}

	const Brawler::D3D12Device& DisplayAdapter::GetD3D12Device() const
	{
		return *(mD3DDevice.Get());
	}

	Brawler::CommandQueue& DisplayAdapter::GetCommandQueue(const Brawler::CommandListType cmdListType)
	{
		assert(mCmdQueueMap.contains(cmdListType));
		return mCmdQueueMap.at(cmdListType);
	}

	const Brawler::CommandQueue& DisplayAdapter::GetCommandQueue(const Brawler::CommandListType cmdListType) const
	{
		assert(mCmdQueueMap.contains(cmdListType));
		return mCmdQueueMap.at(cmdListType);
	}

	bool DisplayAdapter::IsSupportedAdapter() const
	{
		// If mD3DDevice is still nullptr at this point, then CreateD3D12Device()
		// (which is called from the constructor) must have failed to produce a
		// device.
		if (mD3DDevice == nullptr)
			return false;

		// If we are dealing with neither an NVIDIA nor an AMD device, then check for
		// HLSL 6.0 wave operation support. Otherwise, we can just use their APIs.
		if (mVendor != Vendor::NVIDIA && mVendor != Vendor::AMD)
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS1 hlslWaveInfo{};
			mD3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &hlslWaveInfo, sizeof(hlslWaveInfo));

			if (!hlslWaveInfo.WaveOps)
				return false;
		}

		// We require at least Resource Binding Tier 2 to make proper use of bindless
		// SRVs.
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS basicOptions{};
			mD3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &basicOptions, sizeof(basicOptions));

			if (basicOptions.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1)
				return false;
		}

		// Add more capability checks as they are needed here.

		return true;
	}

	void DisplayAdapter::InitializeAdapterInformation()
	{
		DXGI_ADAPTER_DESC adapterDesc{};
		mAdapter->GetDesc(&adapterDesc);

		// Identify the vendor based on the PCI ID from the adapterDesc. These
		// can be found at https://pci-ids.ucw.cz/read/PC?restrict=.
		switch (adapterDesc.VendorId)
		{
		case 0x10de:
			mVendor = Vendor::NVIDIA;
			break;

		// AMD has two PCI IDs, for some reason. (It's probably for pre-/post-
		// acquisition of ATI devices, but I am not sure. Hence, we will just
		// list both here.)
		case 0x1002:
		case 0x1022:
			mVendor = Vendor::AMD;
			break;

		case 0x8086:
			mVendor = Vendor::INTEL;
			break;

		default:
			mVendor = Vendor::UNKNOWN;
			break;
		}

		// Get the amount of VRAM for the display adapter.
		mVRAMSize = adapterDesc.DedicatedVideoMemory;
	}

	void DisplayAdapter::CreateD3D12Device()
	{
		// First, make sure that the device creation succeeds. For some reason, a return
		// value of S_FALSE when passing nullptr for the device COM object means that device
		// creation would succeed.

		if (D3D12CreateDevice(mAdapter.Get(), MINIMUM_FEATURE_LEVEL, __uuidof(Brawler::D3D12Device), nullptr) != S_FALSE)
			return;

		CheckHRESULT(D3D12CreateDevice(mAdapter.Get(), MINIMUM_FEATURE_LEVEL, IID_PPV_ARGS(&mD3DDevice)));
	}

	void DisplayAdapter::InitializeCommandQueues()
	{
		for (std::underlying_type_t<CommandListType> i = 0; i < Util::General::EnumCast(CommandListType::COUNT_OR_ERROR); ++i)
			mCmdQueueMap.emplace(static_cast<CommandListType>(i), static_cast<CommandListType>(i));
	}

	DisplayAdapter& GetDisplayAdapter()
	{
		// Cache the pointer as a thread_local variable. That way, we only pay for the large
		// pointer indirection once.
		thread_local DisplayAdapter& adapter = Brawler::GetApplication().GetRenderer().GetDisplayAdapter();

		return adapter;
	}
}
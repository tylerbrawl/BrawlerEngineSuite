#pragma once

// Fun Fact: You can disable the obnoxious min() and max() macros in the Windows header files with
// this macro definition!
#define NOMINMAX

// What a silly name for a macro...
#define WIN32_LEAN_AND_MEAN

#include <stdexcept>

#include <Windows.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <comdef.h>

namespace Brawler
{
	using D3D12Device = ID3D12Device8;
	using D3D12Heap = ID3D12Heap1;
	using D3D12_RESOURCE_DESC = ::D3D12_RESOURCE_DESC1;
	using D3D12_ROOT_PARAMETER = ::D3D12_ROOT_PARAMETER1;
	using D3D12Resource = ID3D12Resource2;
	using D3D12CommandQueue = ID3D12CommandQueue;
	using D3D12GraphicsCommandList = ID3D12GraphicsCommandList6;
	using D3D12CommandAllocator = ID3D12CommandAllocator;
	using D3D12Fence = ID3D12Fence1;
	using D3D12DescriptorHeap = ID3D12DescriptorHeap;
	using D3D12RootSignature = ID3D12RootSignature;
	using D3D12PipelineState = ID3D12PipelineState;

	using DXGIAdapter = IDXGIAdapter4;
	using DXGIFactory = IDXGIFactory7;
	using DXGISwapChain = IDXGISwapChain4;
	using DXGIOutput = IDXGIOutput6;
}

import Util.General;

#define CheckHRESULT(x)																																																						\
{																																																											\
	const HRESULT hr_ = (x);																																																					\
	if (FAILED(hr_)) [[unlikely]]																																																			\
	{																																																										\
		if constexpr (Util::General::IsDebugModeEnabled())																																													\
		{																																																									\
			_com_error comErr{ hr_ };																																																		\
			throw std::runtime_error{ std::string{"An HRESULT check failed!\n\nHRESULT Returned: "} + Util::General::WStringToString(comErr.ErrorMessage()) +																				\
				"\nFunction: " + __FUNCSIG__ + "\nFile : " + __FILE__ + " (Line Number : " + std::to_string(__LINE__) + ")" };																												\
		}																																																									\
		else																																																								\
			throw std::runtime_error{ "" };																																																	\
	}																																																										\
}
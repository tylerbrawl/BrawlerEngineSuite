#pragma once

// Fun Fact: You can disable the obnoxious min() and max() macros in the Windows header files with
// this macro definition!
#define NOMINMAX

// What a silly name for a macro...
#define WIN32_LEAN_AND_MEAN

#include <stdexcept>

// We need to make sure that the DirectX headers are included before the Windows headers.
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include <Windows.h>
#include <comdef.h>
#include <ShlObj.h>
#include <KnownFolders.h>
#include <dwmapi.h>

namespace Util
{
	namespace General
	{
		std::string WStringToString(const std::wstring_view str);
	}
}

#define CheckHRESULT(x)																																																						\
{																																																											\
	const HRESULT hr = (x);																																																					\
	if (FAILED(hr))																																																							\
	{																																																										\
		_com_error comErr{ hr };																																																			\
		throw std::runtime_error{ std::string{"An HRESULT check failed!\n\nHRESULT Returned: "} + Util::General::WStringToString(comErr.ErrorMessage()) +																					\
				"\nFunction: " + __FUNCSIG__ + "\nFile : " + __FILE__ + " (Line Number : " + std::to_string(__LINE__) + ")"};																												\
	}																																																										\
}

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

	using DXGIAdapter = IDXGIAdapter4;
	using DXGIFactory = IDXGIFactory7;
	using DXGISwapChain = IDXGISwapChain4;
	using DXGIOutput = IDXGIOutput6;
}
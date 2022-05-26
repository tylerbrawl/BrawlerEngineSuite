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
#include <Shlobj.h>

#include "dstorage.h"

#if defined(_DEBUG) || defined(__RELEASE_WITH_DEBUGGING__)
#pragma push_macro("USE_PIX")

#define USE_PIX
#include "pix3.h"
#undef USE_PIX

#pragma pop_macro("USE_PIX")
#endif

namespace Brawler
{
	using D3D12Device = ID3D12Device8;
	using D3D12Heap = ID3D12Heap1;
	using D3D12_RESOURCE_DESC = ::D3D12_RESOURCE_DESC1;
	using D3D12_ROOT_PARAMETER = ::D3D12_ROOT_PARAMETER1;
	using D3D12Resource = ID3D12Resource2;
	using D3D12CommandQueue = ID3D12CommandQueue;
	using D3D12GraphicsCommandList = ID3D12GraphicsCommandList6;
	using D3D12DebugCommandList = ID3D12DebugCommandList2;
	using D3D12CommandAllocator = ID3D12CommandAllocator;
	using D3D12Fence = ID3D12Fence1;
	using D3D12DescriptorHeap = ID3D12DescriptorHeap;
	using D3D12RootSignature = ID3D12RootSignature;
	using D3D12PipelineState = ID3D12PipelineState;
	
	using D3D12Debug = ID3D12Debug3;

	using DXGIAdapter = IDXGIAdapter4;
	using DXGIFactory = IDXGIFactory7;
	using DXGISwapChain = IDXGISwapChain4;
	using DXGIOutput = IDXGIOutput6;
}
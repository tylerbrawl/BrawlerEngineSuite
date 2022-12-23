module;
#include <ranges>
#include <vector>
#include "DxDef.h"

module Brawler.RootSignatureBuilderCreators;

namespace
{
	Brawler::RootSignatures::DescriptorTableInfo CreateBindlessSRVDescriptorTableInfo(const D3D12_SHADER_VISIBILITY visibility)
	{
		// The GPUResourceDescriptorHeap has a segment of 500,000 descriptors reserved for bindless
		// SRVs.
		static constexpr std::size_t BINDLESS_SRV_COUNT = 500000;

		// We currently have 12 different bindless SRV ranges set up. Each SRV range overlaps each other to
		// cover the entire 500,000 bindless SRV segment of the GPUResourceDescriptorHeap.
		static constexpr std::size_t BINDLESS_SRV_RANGE_COUNT = 12;

		// We need to use D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE for bindless SRVs. When
		// static descriptors are used, a promise is made to the driver that the descriptors referenced
		// in a descriptor table will not change after ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable()
		// or ID3D12GraphicsCommandList::SetComputeRootDescriptorTable() is called on the *CPU* timeline.
		//
		// Here is the problem, though: While one thread is calling that function during command list
		// recording, another thread might want to update the descriptors in the bindless SRV segment of the
		// GPUResourceDescriptorHeap. There is no way to guarantee that the thread updating the descriptor
		// heap will finish its work before the thread recording into the command list sets the descriptor
		// table.
		//
		// We assume that the user won't be modifying descriptors in the descriptor heap which the GPU might
		// be accessing. For instance, suppose we want to delete a texture used as material data through a
		// bindless SRV. We would want to wait Util::Engine::MAX_FRAMES_IN_FLIGHT frames before deleting
		// the texture and freeing its sub-allocation within the bindless SRV segment of the descriptor heap.
		// That way, no other thread could overwrite the descriptor until we know that the GPU is finished
		// using it. Even so, the strict wording of the D3D12 specs seem to imply that not even this can
		// safely be done with static descriptors.
		//
		// According to the D3D12 specifications at 
		// https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#restrictions-on-changing-state-pointed-to-by-descriptor-tables,
		// it's fine to update descriptors pointed to by a descriptor table while a command list is being
		// executed iff it can be guaranteed that said descriptors are not referenced by the GPU (e.g., through
		// dynamic indexing). I'm going to assume that this only applies to D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE.
		// (Another part of the specifications directly contradict that statement by stating that no descriptors 
		// pointed to in a descriptor table may be updated during command list execution, even with volatile 
		// descriptors. For my own sanity, I'm going to assume this is incorrect; otherwise, I can't see how 
		// bindless SRVs would even be feasible without incurring massive CPU/GPU synchronization costs.)
		//
		// ================================================================================================================================
		//
		// Regarding the flags used for data volatility (as opposed to descriptor volatility), we also seem to
		// need to use D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE. The reason behind this one is a lot clearer.
		// Suppose we have a resource which has a bindless SRV in the GPUResourceDescriptorHeap, and that we
		// want to write to this resource using a UAV. If we are executing a shader which accesses the bindless
		// SRVs while writing to the resource using the UAV, then the data obviously can't be static. (We assume
		// that the resource being written to is never accessed using a bindless SRV, since it is invalid for a
		// resource to be in the D3D12_RESOURCE_STATE_*_SHADER_RESOURCE state and the
		// D3D12_RESOURCE_STATE_UNORDERED_ACCESS state at the same time.)

		std::vector<CD3DX12_DESCRIPTOR_RANGE1> bindlessSRVTableRanges{};
		bindlessSRVTableRanges.resize(BINDLESS_SRV_RANGE_COUNT);

		for (const auto i : std::views::iota(0ull, BINDLESS_SRV_RANGE_COUNT))
		{
			// All register spaces except for space0 are fair game for bindless SRVs.
			const std::uint32_t currRegisterSpace = static_cast<std::uint32_t>(i + 1);

			bindlessSRVTableRanges[i].Init(
				D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				static_cast<std::uint32_t>(BINDLESS_SRV_COUNT),
				0,
				currRegisterSpace,
				(D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE),

				// Specifying 0 for the offsetInDescriptorsFromTableStart parameter will allow these ranges
				// to overlap each other.
				0
			);
		}

		return Brawler::RootSignatures::DescriptorTableInfo{
			.DescriptorRangeArr{ std::move(bindlessSRVTableRanges) },
			.Visibility = visibility
		};
	}
}

namespace Brawler
{
	namespace RootSignatures
	{
		template <>
		RootSignatureBuilder<Brawler::RootSignatureID::DEFERRED_GEOMETRY_RASTER> CreateRootSignatureBuilder<Brawler::RootSignatureID::DEFERRED_GEOMETRY_RASTER>()
		{
			RootSignatureBuilder<Brawler::RootSignatureID::DEFERRED_GEOMETRY_RASTER> rootSigBuilder{};

			/*
			Root Parameter 0: RootConstants<1> RasterConstants -> Space0[b0];
			*/
			{
				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::DeferredGeometryRaster::RASTER_CONSTANTS>(RootConstantsInfo{
					.Num32BitValues = 1,
					.ShaderRegister = 0,
					.RegisterSpace = 0,
					.Visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX
				});
			}

			/*
			Root Parameter 1: DescriptorTable
			{
				SRV BindlessResources -> Space1-Space?[t0-t499999];
			};
			*/
			{
				static constexpr D3D12_SHADER_VISIBILITY PARAM_1_VISIBILITY = static_cast<D3D12_SHADER_VISIBILITY>(D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_VERTEX | D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL);
				rootSigBuilder.InitializeRootParameter<Brawler::RootParameters::DeferredGeometryRaster::BINDLESS_SRVS>(CreateBindlessSRVDescriptorTableInfo(PARAM_1_VISIBILITY));
			}

			// Static Sampler 0: Anisotropic Wrap Sampler -> Space0[s0];
			{
				CD3DX12_STATIC_SAMPLER_DESC anisotropicWrapSampler{};
				anisotropicWrapSampler.Init(0);
				anisotropicWrapSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;

				rootSigBuilder.AddStaticSampler(std::move(anisotropicWrapSampler));
			}

			// Static Sampler 1: Bilinear Wrap Sampler -> Space0[s1];
			{
				CD3DX12_STATIC_SAMPLER_DESC bilinearWrapSampler{};
				bilinearWrapSampler.Init(1, D3D12_FILTER::D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);
				bilinearWrapSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_PIXEL;

				rootSigBuilder.AddStaticSampler(std::move(bilinearWrapSampler));
			}

			return rootSigBuilder;
		}
	}
}
module;
#include <optional>
#include <variant>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.RootParameterCache;
import Brawler.D3D12.GPUResourceViews;
import Brawler.D3D12.RootDescriptors;
import Brawler.D3D12.I_DescriptorTable;
import Brawler.D3D12.DescriptorHandleInfo;

namespace Brawler
{
	namespace D3D12
	{
		using CachedRootParameterVariant = std::variant<
			// Unassigned Root Parameter
			//
			// We do not cache root constants, since they can be of varying sizes. However,
			// strictly speaking, it would be possible to do so with dynamic polymorphism.
			// The real question is whether or not it would be worth doing that.
			std::monostate,

			// Root Descriptors
			D3D12_GPU_VIRTUAL_ADDRESS,

			// Descriptor Tables
			D3D12_GPU_DESCRIPTOR_HANDLE
		> ;

		template <typename T>
		struct VariantElement
		{
			static_assert(sizeof(T) != sizeof(T));
		};

		template <typename... VariantTypes>
		struct VariantElement<std::variant<VariantTypes...>>
		{
		private:
			using TupleType = std::tuple<VariantTypes...>;

		public:
			template <std::size_t Index>
			using Type = std::tuple_element_t<Index, TupleType>;
		};

		template <typename RootParamType>
		struct CachedRootParameterInfo
		{
			static_assert(sizeof(RootParamType) != sizeof(RootParamType));
		};

		template <std::size_t VariantIndex>
		struct CachedRootParameterInfoInstantiation
		{
			static constexpr std::size_t VARIANT_INDEX = VariantIndex;
		};

		template <>
		struct CachedRootParameterInfo<RootConstantBufferView> : public CachedRootParameterInfoInstantiation<1>
		{
			static D3D12_GPU_VIRTUAL_ADDRESS GetCacheableRootParameterValue(const RootConstantBufferView& rootCBV)
			{
				return rootCBV.GetGPUVirtualAddress();
			}
		};

		template <>
		struct CachedRootParameterInfo<RootShaderResourceView> : public CachedRootParameterInfoInstantiation<1>
		{
			static D3D12_GPU_VIRTUAL_ADDRESS GetCacheableRootParameterValue(const RootShaderResourceView& rootSRV)
			{
				return rootSRV.GetGPUVirtualAddress();
			}
		};

		template <>
		struct CachedRootParameterInfo<RootUnorderedAccessView> : public CachedRootParameterInfoInstantiation<1>
		{
			static D3D12_GPU_VIRTUAL_ADDRESS GetCacheableRootParameterValue(const RootUnorderedAccessView& rootUAV)
			{
				return rootUAV.GetGPUVirtualAddress();
			}
		};

		template <>
		struct CachedRootParameterInfo<I_DescriptorTable> : public CachedRootParameterInfoInstantiation<2>
		{
			static D3D12_GPU_DESCRIPTOR_HANDLE GetCacheableRootParameterValue(const I_DescriptorTable& descriptorTable)
			{
				return descriptorTable.GetDescriptorHandleInfo().HGPUDescriptor;
			}
		};

		template <typename T>
		concept IsCacheableRootParameter = requires ()
		{
			CachedRootParameterInfo<T>::VARIANT_INDEX;
		};

		template <typename RootParamType>
		using CachedRootParameterType = typename VariantElement<CachedRootParameterVariant>::Type<CachedRootParameterInfo<RootParamType>::VARIANT_INDEX>;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class RootParameterCache
		{
		public:
			RootParameterCache() = default;

			RootParameterCache(const RootParameterCache& rhs) = delete;
			RootParameterCache& operator=(const RootParameterCache& rhs) = delete;

			RootParameterCache(RootParameterCache&& rhs) noexcept = default;
			RootParameterCache& operator=(RootParameterCache&& rhs) noexcept = default;

			template <auto RSIdentifier>
				requires std::is_enum_v<decltype(RSIdentifier)>
			void SetRootSignature();

			/// <summary>
			/// Attempts to cache the root parameter specified by rootParam at the index
			/// specified by rootParamIndex. The return value of the function indicates
			/// whether or not caching actually took place, and thus whether or not the
			/// root parameter needs to be set with the D3D12 API.
			/// 
			/// *NOTE*: This function does not do any error checking as to whether or not
			/// a root parameter of a given type can actually be set for the current root
			/// parameter at the specified index. This is verified at compile time by the
			/// highly-templated API of the GPUResourceBinder class.
			/// 
			/// *NOTE*: Caching root constants is currently not supported.
			/// </summary>
			/// <typeparam name="T">
			/// - The type of the root parameter specified by rootParam.
			/// </typeparam>
			/// <param name="rootParamIndex">
			/// - The index at which the specified root parameter will be cached.
			/// </param>
			/// <param name="rootParam">
			/// - The root parameter which is to be cached.
			/// </param>
			/// <returns>
			/// The function returns true if the root parameter was successfully cached
			/// and false otherwise. That is, if the function returns false, then calls to
			/// ID3D12GraphicsCommandList::SetGraphicsRoot*() or
			/// ID3D12GraphicsCommandList::SetComputeRoot*() can be elided for this
			/// particular root parameter assignment.
			/// </returns>
			template <typename T>
				requires IsCacheableRootParameter<T>
			bool UpdateRootParameter(const std::uint32_t rootParamIndex, const T& rootParam);

		private:
			// We need to store the root parameter ID as a raw integer value, rather than
			// as a RootSignatureID, because the RootSignatureID enumeration type is defined
			// in the files generated by the Brawler Shader Compiler.
			std::optional<std::int32_t> mRootSignatureID;

			std::vector<CachedRootParameterVariant> mRootParamArr;
		};
	}
}

// -----------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace RootSignatures
	{
		template <auto RSIdentifier>
		extern consteval std::size_t GetRootParameterCount();
	}
}

namespace Brawler
{
	namespace D3D12
	{
		template <auto RSIdentifier>
			requires std::is_enum_v<decltype(RSIdentifier)>
		void RootParameterCache::SetRootSignature()
		{
			// Do nothing if the specified root signature is already enabled.
			if (mRootSignatureID.has_value() && static_cast<std::underlying_type_t<RSIdentifier>>(*mRootSignatureID) == std::to_underlying(RSIdentifier))
				return;

			mRootSignatureID = std::to_underlying(RSIdentifier);

			mRootParamArr.clear();
			mRootParamArr.resize(Brawler::RootSignatures::GetRootParameterCount<RSIdentifier>());
		}

		template <typename T>
			requires IsCacheableRootParameter<T>
		bool RootParameterCache::UpdateRootParameter(const std::uint32_t rootParamIndex, const T& rootParam)
		{
			assert(rootParamIndex < mRootParamArr.size());
			
			CachedRootParameterType<T> cacheableRootParamValue{ CachedRootParameterInfo<T>::GetCacheableRootParameterValue(rootParam) };

			if (mRootParamArr[rootParamIndex].index() == CachedRootParameterInfo<T>::VARIANT_INDEX)
			{
				// If we already have a cached value for this root parameter, then we need to verify
				// that the value is equivalent. We can do this quickly and efficiently with a 
				// bit-wise value equality check. This is possible because all of the values which
				// we check are either primitives or simple structures.

				const CachedRootParameterType<T>& cachedValue{ std::get<CachedRootParameterType<T>>(mRootParamArr[rootParamIndex]) };

				if (std::memcmp(std::addressof(cachedValue), std::addressof(cacheableRootParamValue), sizeof(CachedRootParameterType<T>)) == 0)
					return false;
			}

			mRootParamArr[rootParamIndex] = std::move(cacheableRootParamValue);
			return true;
		}
	}
}
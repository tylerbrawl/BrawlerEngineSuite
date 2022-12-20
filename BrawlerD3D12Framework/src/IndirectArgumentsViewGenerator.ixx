module;
#include <cstddef>
#include <cassert>
#include <algorithm>
#include "DxDef.h"

export module Brawler.D3D12.IndirectArgumentsViewGenerator;
import Brawler.D3D12.GPUResourceViews;
import Brawler.D3D12.IndirectArgumentsCommandRange;

export namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedClass>
		class IndirectArgumentsViewGenerator
		{
		private:
			using IndirectArgumentsType = DerivedClass::IndirectArgumentsType;

		public:
			IndirectArgumentsViewGenerator() = default;

			IndirectArgumentsViewGenerator(const IndirectArgumentsViewGenerator& rhs) = default;
			IndirectArgumentsViewGenerator& operator=(const IndirectArgumentsViewGenerator& rhs) = default;

			IndirectArgumentsViewGenerator(IndirectArgumentsViewGenerator&& rhs) noexcept = default;
			IndirectArgumentsViewGenerator& operator=(IndirectArgumentsViewGenerator&& rhs) noexcept = default;

			StructuredBufferShaderResourceView CreateShaderResourceView() const;
			StructuredBufferShaderResourceView CreateShaderResourceView(const IndirectArgumentsCommandRange& commandRange) const;

			RawBufferShaderResourceView CreateRawShaderResourceView() const;

			StructuredBufferUnorderedAccessView CreateUnorderedAccessView() const;
			StructuredBufferUnorderedAccessView CreateUnorderedAccessView(const IndirectArgumentsCommandRange& commandRange) const;

			RawBufferUnorderedAccessView CreateRawUnorderedAccessView() const;

		private:
			IndirectArgumentsCommandRange GetCompleteCommandRange() const;

			DerivedClass& GetDerivedClass();
			const DerivedClass& GetDerivedClass() const;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename DerivedClass>
		StructuredBufferShaderResourceView IndirectArgumentsViewGenerator<DerivedClass>::CreateShaderResourceView() const
		{
			return CreateShaderResourceView(GetCompleteCommandRange());
		}

		template <typename DerivedClass>
		StructuredBufferShaderResourceView IndirectArgumentsViewGenerator<DerivedClass>::CreateShaderResourceView(const IndirectArgumentsCommandRange& commandRange) const
		{
			assert(commandRange.FirstCommand + commandRange.NumCommands <= GetDerivedClass().GetCommandCount() && "ERROR: An out-of-bounds IndirectArgumentsCommandRange was specified in a call to IndirectArgumentsViewGenerator::CreateShaderResourceView()!");
			
			const std::size_t offsetFromBufferStartInElements = ((GetDerivedClass().GetOffsetFromBufferStart() / sizeof(IndirectArgumentsType)) + commandRange.FirstCommand);

			return StructuredBufferShaderResourceView{ GetDerivedClass().GetGPUResource(), D3D12_BUFFER_SRV{
				.FirstElement = offsetFromBufferStartInElements,
				.NumElements = commandRange.NumCommands,
				.StructureByteStride = sizeof(IndirectArgumentsType)
			} };
		}

		template <typename DerivedClass>
		RawBufferShaderResourceView IndirectArgumentsViewGenerator<DerivedClass>::CreateRawShaderResourceView() const
		{
			const std::size_t offsetFromBufferStartInR32Elements = (GetDerivedClass().GetOffsetFromBufferStart() / sizeof(std::uint32_t));

			const std::size_t numBytesTotal = (GetDerivedClass().GetCommandCount() * sizeof(IndirectArgumentsType));
			const std::size_t numR32ElementsTotal = (numBytesTotal / sizeof(std::uint32_t)) + std::min<std::size_t>(numBytesTotal % sizeof(std::uint32_t), 1);

			return RawBufferShaderResourceView{ GetDerivedClass().GetGPUResource(), D3D12_BUFFER_SRV{
				.FirstElement = offsetFromBufferStartInR32Elements,
				.NumElements = numR32ElementsTotal,
				.Flags = D3D12_BUFFER_SRV_FLAGS::D3D12_BUFFER_SRV_FLAG_RAW
			} };
		}

		template <typename DerivedClass>
		StructuredBufferUnorderedAccessView IndirectArgumentsViewGenerator<DerivedClass>::CreateUnorderedAccessView() const
		{
			return CreateUnorderedAccessView(GetCompleteCommandRange());
		}

		template <typename DerivedClass>
		StructuredBufferUnorderedAccessView IndirectArgumentsViewGenerator<DerivedClass>::CreateUnorderedAccessView(const IndirectArgumentsCommandRange& commandRange) const
		{
			assert(commandRange.FirstCommand + commandRange.NumCommands <= GetDerivedClass().GetCommandCount() && "ERROR: An out-of-bounds IndirectArgumentsCommandRange was specified in a call to IndirectArgumentsViewGenerator::CreateUnorderedAccessView()!");

			const std::size_t offsetFromBufferStartInElements = ((GetDerivedClass().GetOffsetFromBufferStart() / sizeof(IndirectArgumentsType)) + commandRange.FirstCommand);

			return StructuredBufferUnorderedAccessView{ GetDerivedClass().GetGPUResource(), D3D12_BUFFER_UAV{
				.FirstElement = offsetFromBufferStartInElements,
				.NumElements = commandRange.NumCommands,
				.StructureByteStride = sizeof(IndirectArgumentsType)
			} };
		}

		template <typename DerivedClass>
		RawBufferUnorderedAccessView IndirectArgumentsViewGenerator<DerivedClass>::CreateRawUnorderedAccessView() const
		{
			const std::size_t offsetFromBufferStartInR32Elements = (GetDerivedClass().GetOffsetFromBufferStart() / sizeof(std::uint32_t));

			const std::size_t numBytesTotal = (GetDerivedClass().GetCommandCount() * sizeof(IndirectArgumentsType));
			const std::size_t numR32ElementsTotal = (numBytesTotal / sizeof(std::uint32_t)) + std::min<std::size_t>(numBytesTotal % sizeof(std::uint32_t), 1);

			return RawBufferUnorderedAccessView{ GetDerivedClass().GetGPUResource(), D3D12_BUFFER_UAV{
				.FirstElement = offsetFromBufferStartInR32Elements,
				.NumElements = numR32ElementsTotal,
				.Flags = D3D12_BUFFER_UAV_FLAGS::D3D12_BUFFER_UAV_FLAG_RAW
			} };
		}

		template <typename DerivedClass>
		IndirectArgumentsCommandRange IndirectArgumentsViewGenerator<DerivedClass>::GetCompleteCommandRange() const
		{
			return IndirectArgumentsCommandRange{
				.FirstCommand = 0,
				.NumCommands = GetDerivedClass().GetCommandCount()
			};
		}

		template <typename DerivedClass>
		DerivedClass& IndirectArgumentsViewGenerator<DerivedClass>::GetDerivedClass()
		{
			return *(static_cast<DerivedClass*>(this));
		}

		template <typename DerivedClass>
		const DerivedClass& IndirectArgumentsViewGenerator<DerivedClass>::GetDerivedClass() const
		{
			return *(static_cast<const DerivedClass*>(this));
		}
	}
}
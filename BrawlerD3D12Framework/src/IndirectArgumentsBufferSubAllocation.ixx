module;
#include <cstddef>
#include <limits>
#include <span>
#include <cassert>
#include <numeric>

export module Brawler.D3D12.IndirectArgumentsBufferSubAllocation;
export import Brawler.D3D12.IndirectArgumentsCommandRange;
import Brawler.D3D12.IndirectArgumentsViewGenerator;
import Brawler.D3D12.GPUResourceViews;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.CommandSignatures.CommandSignatureID;
import Brawler.CommandSignatures.CommandSignatureDefinition;
import Util.Math;

namespace Brawler
{
	namespace D3D12
	{
		static constexpr std::size_t DYNAMIC_EXTENT = std::dynamic_extent;

		template <std::size_t CommandCount>
		class CommandCountContainer
		{
		public:
			CommandCountContainer() = default;

			static consteval std::size_t GetCommandCount()
			{
				return CommandCount;
			}
		};

		template <>
		class CommandCountContainer<DYNAMIC_EXTENT>
		{
		public:
			CommandCountContainer() = default;

			explicit CommandCountContainer(const std::size_t numCommands) :
				mNumCommands(numCommands)
			{}

			std::size_t GetCommandCount() const
			{
				return mNumCommands;
			}

		private:
			std::size_t mNumCommands;
		};
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		template <CommandSignatures::CommandSignatureID CSIdentifier, std::size_t CommandCount = DYNAMIC_EXTENT>
		class IndirectArgumentsBufferSubAllocation final : public I_BufferSubAllocation, public IndirectArgumentsViewGenerator<IndirectArgumentsBufferSubAllocation<CSIdentifier, CommandCount>>, private CommandCountContainer<CommandCount>
		{
		private:
			static constexpr bool DOES_COMMAND_SIGNATURE_HAVE_ROOT_SIGNATURE = CommandSignatures::DoesCommandSignatureHaveAssociatedRootSignature<CSIdentifier>();

		public:
			using IndirectArgumentsType = CommandSignatures::IndirectArgumentsType<CSIdentifier>;

		public:
			IndirectArgumentsBufferSubAllocation() requires (CommandCount != DYNAMIC_EXTENT) = default;

			explicit IndirectArgumentsBufferSubAllocation(const std::size_t numCommands = 1) requires (CommandCount == DYNAMIC_EXTENT) :
				I_BufferSubAllocation(),
				IndirectArgumentsViewGenerator<IndirectArgumentsBufferSubAllocation<CSIdentifier, CommandCount>>(),
				CommandCountContainer<CommandCount>(numCommands)
			{}

			IndirectArgumentsBufferSubAllocation(const IndirectArgumentsBufferSubAllocation& rhs) = delete;
			IndirectArgumentsBufferSubAllocation& operator=(const IndirectArgumentsBufferSubAllocation& rhs) = delete;

			IndirectArgumentsBufferSubAllocation(IndirectArgumentsBufferSubAllocation&& rhs) noexcept = default;
			IndirectArgumentsBufferSubAllocation& operator=(IndirectArgumentsBufferSubAllocation&& rhs) noexcept = default;

			std::size_t GetSubAllocationSize() const override;
			std::size_t GetRequiredDataPlacementAlignment() const override;

			void WriteIndirectArgumentsData(const std::size_t startElementIndex, const std::span<const IndirectArgumentsType> srcDataSpan) const;
			void ReadIndirectArgumentsData(const std::size_t startElementIndex, const std::span<IndirectArgumentsType> destDataSpan) const;

			std::size_t GetCommandCount() const;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <CommandSignatures::CommandSignatureID CSIdentifier, std::size_t CommandCount>
		std::size_t IndirectArgumentsBufferSubAllocation<CSIdentifier, CommandCount>::GetSubAllocationSize() const
		{
			// We want to align the generated sub-allocation size to a multiple of 4 bytes. That way,
			// raw byte address buffer view descriptions can always be valid. (Otherwise, we might
			// specify an element range in a D3D12_BUFFER_SRV or D3D12_BUFFER_UAV which goes outside
			// of the buffer sub-allocation.)
			
			// Try to solve for this value at compile time, if possible.
			if constexpr (CommandCount == DYNAMIC_EXTENT)
			{
				static constexpr std::size_t SUB_ALLOCATION_SIZE = Util::Math::AlignToPowerOfTwo(sizeof(IndirectArgumentsType) * CommandCountContainer<CommandCount>::GetCommandCount(), sizeof(std::uint32_t));
				return SUB_ALLOCATION_SIZE;
			}
			else
				return Util::Math::AlignToPowerOfTwo(sizeof(IndirectArgumentsType) * GetCommandCount(), sizeof(std::uint32_t));
		}

		template <CommandSignatures::CommandSignatureID CSIdentifier, std::size_t CommandCount>
		std::size_t IndirectArgumentsBufferSubAllocation<CSIdentifier, CommandCount>::GetRequiredDataPlacementAlignment() const
		{
			// According to the MSDN, both indirect argument commands and indirect argument counts
			// must be aligned to a 4-byte offset from the start of the buffer. (Source: 
			// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-executeindirect#remarks)
			//
			// However, we want to be able to use the indirect argument buffer data in both structured
			// buffer views, which require a data placement alignment of the size of the structure,
			// and raw byte address buffers, which require a data placement alignment of 16 bytes.
			//
			// We thus choose the lowest possible placement alignment which satisfies both constraints
			// at compile time.
			static constexpr std::size_t DATA_PLACEMENT_ALIGNMENT = std::lcm(sizeof(IndirectArgumentsType), 16);
			return DATA_PLACEMENT_ALIGNMENT;
		}

		template <CommandSignatures::CommandSignatureID CSIdentifier, std::size_t CommandCount>
		void IndirectArgumentsBufferSubAllocation<CSIdentifier, CommandCount>::WriteIndirectArgumentsData(const std::size_t startElementIndex, const std::span<const IndirectArgumentsType> srcDataSpan) const
		{
			assert(startElementIndex + srcDataSpan.size() <= GetCommandCount() && "ERROR: An out-of-bounds destination data range was specified in a call to IndirectArgumentsBufferSubAllocation::WriteIndirectArgumentsData()!");

			const std::size_t offsetFromSubAllocationStart = (sizeof(IndirectArgumentsType) * startElementIndex);
			WriteToBuffer(srcDataSpan, offsetFromSubAllocationStart);
		}

		template <CommandSignatures::CommandSignatureID CSIdentifier, std::size_t CommandCount>
		void IndirectArgumentsBufferSubAllocation<CSIdentifier, CommandCount>::ReadIndirectArgumentsData(const std::size_t startElementIndex, const std::span<IndirectArgumentsType> destDataSpan) const
		{
			assert(startElementIndex + destDataSpan.size() <= GetCommandCount() && "ERROR: An out-of-bounds source data range was specified in a call to IndirectArgumentsBufferSubAllocation::ReadIndirectArgumentsData()!");

			const std::size_t offsetFromSubAllocationStart = (sizeof(IndirectArgumentsType) * startElementIndex);
			ReadFromBuffer(destDataSpan, offsetFromSubAllocationStart);
		}

		template <CommandSignatures::CommandSignatureID CSIdentifier, std::size_t CommandCount>
		std::size_t IndirectArgumentsBufferSubAllocation<CSIdentifier, CommandCount>::GetCommandCount() const
		{
			return CommandCountContainer<CommandCount>::GetCommandCount();
		}
	}
}
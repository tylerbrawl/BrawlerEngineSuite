module;
#include <concepts>

export module Brawler.D3D12.GenericBufferSnapshots;
import Brawler.D3D12.I_BufferSnapshot;
import Brawler.D3D12.I_BufferSubAllocation;
import Brawler.D3D12.UAVCounterSubAllocation;


namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
			requires std::derived_from<T, I_BufferSubAllocation>
		class GenericBufferSnapshot final : public I_BufferSnapshot
		{
		public:
			explicit GenericBufferSnapshot(const T& bufferSubAllocation);

			GenericBufferSnapshot(const GenericBufferSnapshot& rhs) = default;
			GenericBufferSnapshot& operator=(const GenericBufferSnapshot& rhs) = default;

			GenericBufferSnapshot(GenericBufferSnapshot&& rhs) noexcept = default;
			GenericBufferSnapshot& operator=(GenericBufferSnapshot&& rhs) noexcept = default;
		};
	}
}

// ------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
			requires std::derived_from<T, I_BufferSubAllocation>
		GenericBufferSnapshot<T>::GenericBufferSnapshot(const T& bufferSubAllocation) :
			I_BufferSnapshot(bufferSubAllocation)
		{}
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		using UAVCounterSnapshot = GenericBufferSnapshot<UAVCounterSubAllocation>;
	}
}
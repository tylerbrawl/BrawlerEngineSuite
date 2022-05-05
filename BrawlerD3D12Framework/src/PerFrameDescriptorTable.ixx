module;
#include "DxDef.h"

export module Brawler.D3D12.PerFrameDescriptorTable;
import Brawler.D3D12.I_DescriptorTable;

namespace Brawler
{
	namespace D3D12
	{
		class GPUResourceDescriptorHeap;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class PerFrameDescriptorTable final : public I_DescriptorTable
		{
		private:
			struct InitializationInfo
			{
				DescriptorHandleInfo HandleInfo;
				std::uint64_t CurrentFrameNumber;
			};

		private:
			friend class GPUResourceDescriptorHeap;

		private:
			explicit PerFrameDescriptorTable(InitializationInfo&& initInfo);

		public:
			PerFrameDescriptorTable(const PerFrameDescriptorTable& rhs) = default;
			PerFrameDescriptorTable& operator=(const PerFrameDescriptorTable& rhs) = default;

			PerFrameDescriptorTable(PerFrameDescriptorTable&& rhs) noexcept = default;
			PerFrameDescriptorTable& operator=(PerFrameDescriptorTable&& rhs) noexcept = default;

			bool IsDescriptorTableValid() const override;

		private:
#ifdef _DEBUG
			std::uint64_t mCreationFrameNum;
#endif // _DEBUG
		};
	}
}
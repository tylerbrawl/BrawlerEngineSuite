module;
#include "DxDef.h"

export module Brawler.D3D12.PerFrameDescriptorTable;
import Brawler.D3D12.I_DescriptorTable;

export namespace Brawler
{
	namespace D3D12
	{
		class PerFrameDescriptorTable final : public I_DescriptorTable
		{
		public:
			struct InitializationInfo
			{
				DescriptorHandleInfo HandleInfo;
				std::uint64_t CurrentFrameNumber;
			};

		public:
			explicit PerFrameDescriptorTable(InitializationInfo&& initInfo);

			PerFrameDescriptorTable(const PerFrameDescriptorTable& rhs) = default;
			PerFrameDescriptorTable& operator=(const PerFrameDescriptorTable& rhs) = default;

			PerFrameDescriptorTable(PerFrameDescriptorTable&& rhs) noexcept = default;
			PerFrameDescriptorTable& operator=(PerFrameDescriptorTable&& rhs) noexcept = default;

			bool IsDescriptorTableValid() const override;

		private:
			std::uint64_t mCreationFrameNum;
		};
	}
}
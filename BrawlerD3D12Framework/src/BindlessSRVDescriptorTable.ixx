module;

export module Brawler.D3D12.BindlessSRVDescriptorTable;
import Brawler.D3D12.I_DescriptorTable;

export namespace Brawler
{
	namespace D3D12
	{
		class BindlessSRVDescriptorTable final : public I_DescriptorTable
		{
		public:
			BindlessSRVDescriptorTable();

			BindlessSRVDescriptorTable(const BindlessSRVDescriptorTable& rhs) = default;
			BindlessSRVDescriptorTable& operator=(const BindlessSRVDescriptorTable& rhs) = default;

			BindlessSRVDescriptorTable(BindlessSRVDescriptorTable&& rhs) noexcept = default;
			BindlessSRVDescriptorTable& operator=(BindlessSRVDescriptorTable&& rhs) noexcept = default;

			bool IsDescriptorTableValid() const override;
		};
	}
}
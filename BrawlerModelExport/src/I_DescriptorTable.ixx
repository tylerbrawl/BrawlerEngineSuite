module;

export module Brawler.D3D12.GPUResourceDescriptors.I_DescriptorTable;
import Brawler.D3D12.GPUResourceDescriptors.DescriptorHandleInfo;

export namespace Brawler
{
	namespace D3D12
	{
		class I_DescriptorTable
		{
		protected:
			I_DescriptorTable() = default;
			explicit I_DescriptorTable(DescriptorHandleInfo&& handleInfo);

		public:
			virtual ~I_DescriptorTable() = default;

			I_DescriptorTable(const I_DescriptorTable& rhs) = default;
			I_DescriptorTable& operator=(const I_DescriptorTable& rhs) = default;

			I_DescriptorTable(I_DescriptorTable&& rhs) noexcept = default;
			I_DescriptorTable& operator=(I_DescriptorTable&& rhs) noexcept = default;

			const DescriptorHandleInfo& GetDescriptorHandleInfo() const;
			
		protected:
			virtual bool IsDescriptorTableValid() const = 0;

		private:
			DescriptorHandleInfo mHandleInfo;
		};
	}
}
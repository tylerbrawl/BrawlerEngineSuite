module;

export module Tests.ResourceStateTrackingTestModule;
import Brawler.D3D12.I_RenderModule;

namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphBuilder;
	}
}

export namespace Tests
{
	class ResourceStateTrackingTestModule final : public Brawler::D3D12::I_RenderModule
	{
	public:
		ResourceStateTrackingTestModule() = default;

		ResourceStateTrackingTestModule(const ResourceStateTrackingTestModule& rhs) = delete;
		ResourceStateTrackingTestModule& operator=(const ResourceStateTrackingTestModule& rhs) = delete;

		ResourceStateTrackingTestModule(ResourceStateTrackingTestModule&& rhs) noexcept = default;
		ResourceStateTrackingTestModule& operator=(ResourceStateTrackingTestModule&& rhs) noexcept = default;

	protected:
		void BuildFrameGraph(Brawler::D3D12::FrameGraphBuilder& builder) override;

	private:
		void PerformTest1(Brawler::D3D12::FrameGraphBuilder& builder) const;
		void PerformTest2(Brawler::D3D12::FrameGraphBuilder& builder) const;
		void PerformTest3(Brawler::D3D12::FrameGraphBuilder& builder) const;
	};
}
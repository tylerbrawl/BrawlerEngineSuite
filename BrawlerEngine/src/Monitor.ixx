module;
#include <DxDef.h>

export module Brawler.Monitor;

export namespace Brawler
{
	class Monitor
	{
	public:
		explicit Monitor(Microsoft::WRL::ComPtr<Brawler::DXGIOutput>&& dxgiOutputPtr);

		Monitor(const Monitor& rhs) = delete;
		Monitor& operator=(const Monitor& rhs) = delete;

		Monitor(Monitor&& rhs) noexcept = default;
		Monitor& operator=(Monitor&& rhs) noexcept = default;

		Brawler::DXGIOutput& GetDXGIOutput() const;
		HMONITOR GetMonitorHandle() const;

	private:
		Microsoft::WRL::ComPtr<Brawler::DXGIOutput> mDXGIOutputPtr;
		Brawler::DXGI_OUTPUT_DESC mOutputDesc;
	};
}
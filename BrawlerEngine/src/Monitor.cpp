module;
#include <cassert>
#include <vector>
#include <memory>
#include <span>
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

module Brawler.Monitor;
import Util.General;

namespace Brawler
{
	Monitor::Monitor(Microsoft::WRL::ComPtr<Brawler::DXGIOutput>&& dxgiOutputPtr) :
		mDXGIOutputPtr(std::move(dxgiOutputPtr)),
		mOutputDesc(),
		mDisplayModeArr()
	{
		assert(mDXGIOutputPtr != nullptr);
		UpdateMonitorInformation();
	}

	Brawler::DXGIOutput& Monitor::GetDXGIOutput() const
	{
		return *(mDXGIOutputPtr.Get());
	}

	HMONITOR Monitor::GetMonitorHandle() const
	{
		return mOutputDesc.Monitor;
	}

	void Monitor::UpdateMonitorInformation()
	{
		Util::General::CheckHRESULT(mDXGIOutputPtr->GetDesc1(&mOutputDesc));

		ResetDisplayModeList();
	}

	const Brawler::DXGI_OUTPUT_DESC& Monitor::GetOutputDescription() const
	{
		return mOutputDesc;
	}

	DXGI_FORMAT Monitor::GetPreferredSwapChainFormat() const
	{
		// D3D12 only supports the flip-model style of presentation, which in turn only supports
		// three back buffer formats:
		//
		//   - DXGI_FORMAT_R16G16B16A16_FLOAT
		//   - DXGI_FORMAT_B8G8R8A8_UNORM
		//   - DXGI_FORMAT_R8G8B8A8_UNORM
		//
		// Note that the _SRGB formats (where applicable) are not included in this list. However,
		// for render target views created for back buffers only, it is legal to convert from these
		// formats to their respective _SRGB variant, should they have one.

		switch (mOutputDesc.ColorSpace)
		{
		case DXGI_COLOR_SPACE_TYPE::DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;

		case DXGI_COLOR_SPACE_TYPE::DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
			return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;

		default: [[unlikely]]
		{
			// If we find an unknown color space, then I guess we can just use the same format
			// as with HDR Rec.2020.
			return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
		}
	}

	std::span<const Brawler::DXGI_MODE_DESC> Monitor::GetDisplayModeSpan() const
	{
		return std::span<const Brawler::DXGI_MODE_DESC>{ mDisplayModeArr };
	}

	void Monitor::ResetDisplayModeList()
	{
		const auto getFormatDisplayModesLambda = [this]<DXGI_FORMAT BufferFormat>()
		{
			while (true)
			{
				std::vector<Brawler::DXGI_MODE_DESC> formatModeDescArr{};
				std::uint32_t modeCount = 0;

				HRESULT hr = mDXGIOutputPtr->GetDisplayModeList1(
					BufferFormat,
					0,
					&modeCount,
					nullptr
				);

				if (FAILED(hr)) [[unlikely]]
					return formatModeDescArr;

				formatModeDescArr.resize(modeCount);

				hr = mDXGIOutputPtr->GetDisplayModeList1(
					BufferFormat,
					0,
					&modeCount,
					formatModeDescArr.data()
				);

				switch (hr)
				{
				case S_OK: [[likely]]
					return formatModeDescArr;

				case DXGI_ERROR_MORE_DATA:
					break;

				default:
					return std::vector<Brawler::DXGI_MODE_DESC>{};
				}
			}
		};

		std::vector<Brawler::DXGI_MODE_DESC> sdrDisplayModeArr{ getFormatDisplayModesLambda.operator()<DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM>() };
		std::vector<Brawler::DXGI_MODE_DESC> hdrDisplayModeArr{ getFormatDisplayModesLambda.operator()<DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT>() };

		mDisplayModeArr.reserve(sdrDisplayModeArr.size() + hdrDisplayModeArr.size());

		for (auto&& sdrDisplayMode : sdrDisplayModeArr)
			mDisplayModeArr.push_back(std::move(sdrDisplayMode));

		for (auto&& hdrDisplayMode : hdrDisplayModeArr)
			mDisplayModeArr.push_back(std::move(hdrDisplayMode));
	}
}
module;
#include <array>
#include <limits>
#include "DxDef.h"

module Tests.RootSignatureTest;
import Util.Engine;
import Util.General;

namespace Tests
{
	/*
	As stated at https://githubhelp.com/gpuweb/gpuweb, the ID3D12RootSignature object *NEEDS* to be created at runtime.
	This is because the driver is allowed to re-define the root signature internally in order to improve performance.

	(Also, this website offers a good, if lengthy, discussion of some of the differences between Vulkan and DirectX 12.
	I may want to check that out in the future.)
	*/

	void RunRootSignatureTests()
	{
		static constexpr std::array<std::uint8_t, 384> SERIALIZED_ROOT_SIGNATURE_VERSION_1_0{ 0x44,0x58,0x42,0x43,0x57,0xDD,0xFB,0x6E,0xC9,0x6C,0xD8,0x86,0x3A,0xDA,0x2D,0x0E,0x7D,0xB0,0x9F,0x28,0x01,0x00,0x00,0x00,0x80,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x52,0x54,0x53,0x30,0x54,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x20,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xA8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xB0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xB8,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0xD4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x0C,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x55,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x7F,0x7F,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSigObject{};
		Util::General::CheckHRESULT(Util::Engine::GetD3D12Device().CreateRootSignature(
			0,
			SERIALIZED_ROOT_SIGNATURE_VERSION_1_0.data(),
			SERIALIZED_ROOT_SIGNATURE_VERSION_1_0.size(),
			IID_PPV_ARGS(&rootSigObject)
		));

		int breakHere = -1;
	}
}
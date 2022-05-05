module;
#include <array>
#include "DxDef.h"

export module Brawler.ThreadLocalResources;

export namespace Brawler
{
	// Add data which each WorkerThread should keep its own version of to this structure.

	struct ThreadLocalResources
	{
		Microsoft::WRL::ComPtr<IDxcCompiler3> DXCShaderCompiler;
		Microsoft::WRL::ComPtr<IDxcUtils> DXCUtils;

		ThreadLocalResources() :
			DXCShaderCompiler(nullptr),
			DXCUtils(nullptr)
		{
			CheckHRESULT(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DXCShaderCompiler)));
			CheckHRESULT(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DXCUtils)));
		}
	};
}
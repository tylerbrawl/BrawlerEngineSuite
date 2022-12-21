module;
#include "DxDef.h"

export module Brawler.ThreadLocalResources;

export namespace Brawler
{
	// Add data which each WorkerThread should keep its own version of to this structure.

	struct ThreadLocalResources
	{
		Microsoft::WRL::ComPtr<IDxcCompiler3> DXCShaderCompiler;
		Microsoft::WRL::ComPtr<IDxcUtils> DXCUtils;

		ThreadLocalResources();
	};
}
module;
#include <DxDef.h>

export module Util.ZSTD;

export namespace Util
{
	namespace ZSTD
	{
		HRESULT ZSTDErrorToHRESULT(const std::size_t zstdFunctionError);
	}
}
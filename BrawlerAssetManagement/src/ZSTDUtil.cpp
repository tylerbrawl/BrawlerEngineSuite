module;
#include <zstd.h>
#include <zstd_errors.h>
#include <DxDef.h>

module Util.ZSTD;

namespace Util
{
	namespace ZSTD
	{
		HRESULT ZSTDErrorToHRESULT(const std::size_t zstdFunctionError)
		{
			const ZSTD_ErrorCode convertedCode = ZSTD_getErrorCode(zstdFunctionError);

			switch (convertedCode)
			{
			case ZSTD_ErrorCode::ZSTD_error_no_error:
				return S_OK;

			case ZSTD_ErrorCode::ZSTD_error_corruption_detected:
			case ZSTD_ErrorCode::ZSTD_error_parameter_unsupported:
			case ZSTD_ErrorCode::ZSTD_error_srcSize_wrong:
			case ZSTD_ErrorCode::ZSTD_error_dstBuffer_null:
				return E_INVALIDARG;

			case ZSTD_ErrorCode::ZSTD_error_parameter_outOfBound:
				return E_BOUNDS;

			case ZSTD_ErrorCode::ZSTD_error_memory_allocation:
				return E_OUTOFMEMORY;

			case ZSTD_ErrorCode::ZSTD_error_dstSize_tooSmall:
				return E_NOT_SUFFICIENT_BUFFER;

			default:
				return E_FAIL;
			}
		}
	}
}
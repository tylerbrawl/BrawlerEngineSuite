#pragma once

// Fun Fact: You can disable the obnoxious min() and max() macros in the Windows header files with
// this macro definition!
#define NOMINMAX

// What a silly name for a macro...
#define WIN32_LEAN_AND_MEAN

#include <stdexcept>

// We need to define WIN32_NO_STATUS in order to #include <ntstatus.h>, but we #undef it so that
// <ntstatus.h> creates the first definition of it.
#define WIN32_NO_STATUS
#include <Windows.h>
#undef WIN32_NO_STATUS

#include <ntstatus.h>
#include <bcrypt.h>

namespace Util
{
	namespace General
	{
		std::string WStringToString(const std::wstring_view str);
	}
}

#define CheckHRESULT(x)																																																						\
{																																																											\
	const HRESULT hr = (x);																																																					\
	if (FAILED(hr))																																																							\
	{																																																										\
		_com_error comErr{ hr };																																																			\
		throw std::runtime_error{ std::string{"An HRESULT check failed!\n\nHRESULT Returned: "} + Util::General::WStringToString(comErr.ErrorMessage()) +																					\
				"\nFunction: " + __FUNCSIG__ + "\nFile : " + __FILE__ + " (Line Number : " + std::to_string(__LINE__) + ")"};																												\
	}																																																										\
}
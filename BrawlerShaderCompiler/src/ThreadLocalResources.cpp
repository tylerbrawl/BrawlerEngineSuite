module;
#include "DxDef.h"

module Brawler.ThreadLocalResources;

namespace Brawler
{
	ThreadLocalResources::ThreadLocalResources() :
		DXCShaderCompiler(nullptr),
		DXCUtils(nullptr)
	{
		// I've seen some pretty weird bugs with the MSVC in my time using C++20
		// modules, but I think the one I'm about to explain here takes the cake.
		//
		// For some inexplicable reason, if DxcCreateInstance() is called only
		// within .ixx files, then upon compiling the program and running it, you
		// will find that it seems to always return E_NOINTERFACE. This occurs even
		// if the dxcompiler.dll and dxil.dll libraries are properly loaded.
		//
		// If you call these functions within any .cpp file, however, then everything works
		// out fine, even if you also call DxcCreateInstance() from .ixx files. I don't
		// know if I should blame the MSVC team, the DirectX Shader Compiler team, or
		// somebody else unfortunate enough to be picked on, but something's not right
		// here, and it's not our fault.

		CheckHRESULT(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DXCShaderCompiler)));
		CheckHRESULT(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DXCUtils)));
	}
}
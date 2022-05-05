module;
#include <string>
#include <assimp/scene.h>
#include "DxDef.h"

module Util.General;
import Brawler.Application;
import Brawler.SceneLoader;

namespace Util
{
	namespace General
	{
		std::wstring StringToWString(const std::string_view str)
		{
			if (str.empty())
				return std::wstring{};

			std::int32_t wideCharsNeeded = MultiByteToWideChar(
				CP_UTF8,
				0,
				str.data(),
				-1,
				nullptr,
				0
			);

			if (!wideCharsNeeded)
				throw std::runtime_error{ "MultiByteToWideChar() failed to get the number of required characters!" };

			wchar_t* wStrBuffer = new wchar_t[wideCharsNeeded];
			ZeroMemory(wStrBuffer, wideCharsNeeded * sizeof(wchar_t));

			std::int32_t result = MultiByteToWideChar(
				CP_UTF8,
				0,
				str.data(),
				-1,
				wStrBuffer,
				wideCharsNeeded
			);

			if (result != wideCharsNeeded)
				throw std::runtime_error{ "MultiByteToWideChar() failed to convert a string!" };

			std::wstring wideStr{ wStrBuffer };
			delete[] wStrBuffer;

			return wideStr;
		}

		std::string WStringToString(const std::wstring_view wStr)
		{
			if (wStr.empty())
				return std::string{};

			std::int32_t byteCharsNeeded = WideCharToMultiByte(
				CP_UTF8,
				0,
				wStr.data(),
				-1,
				nullptr,
				0,
				nullptr,
				nullptr
			);

			if (!byteCharsNeeded)
				throw std::runtime_error{ "WideCharToMultiByte() failed to get the number of required characters!" };

			char* strBuffer = new char[byteCharsNeeded];
			ZeroMemory(strBuffer, byteCharsNeeded);

			std::int32_t result = WideCharToMultiByte(
				CP_UTF8,
				0,
				wStr.data(),
				-1,
				strBuffer,
				byteCharsNeeded,
				nullptr,
				nullptr
			);

			std::string str{ strBuffer };
			delete[] strBuffer;

			return str;
		}

		const aiScene& GetScene()
		{
			thread_local const aiScene& scene{ Brawler::GetApplication().GetSceneLoader().GetScene() };
			return scene;
		}

		const Brawler::AppParams& GetLaunchParameters()
		{
			thread_local const Brawler::AppParams& launchParams{ Brawler::GetApplication().GetLaunchParameters() };
			return launchParams;
		}
	}
}
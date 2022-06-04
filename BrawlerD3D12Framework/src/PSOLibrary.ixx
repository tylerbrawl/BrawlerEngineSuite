module;
#include "DxDef.h"

export module Brawler.PSODatabase:PSOLibrary;

export namespace Brawler
{
	namespace D3D12
	{
		class PSOLibrary
		{
		public:
			PSOLibrary() = default;

			PSOLibrary(const PSOLibrary& rhs) = delete;
			PSOLibrary& operator=(const PSOLibrary& rhs) = delete;

			PSOLibrary(PSOLibrary&& rhs) noexcept = default;
			PSOLibrary& operator=(PSOLibrary&& rhs) noexcept = default;

		private:

		};
	}
}
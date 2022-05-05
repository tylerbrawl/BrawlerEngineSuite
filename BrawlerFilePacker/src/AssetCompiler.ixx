module;

export module Brawler.AssetCompiler;

// An ugly bug somewhere in either the MSVC or Microsoft's STL means that we have to
// include this little line here. Otherwise, std::filesystem complains about
// std::partial_ordering not being defined.

export import <compare>;

// What's even worse than that, however, is that we get a bunch of macro re-definition
// warnings for files which both import this file somehow and #include "Win32Def.h."
// So, if you see a bunch of weird #pragma warning directives everywhere, this is why.

import Brawler.BCALinker;

export namespace Brawler
{
	struct AssetCompilerContext;
}

export namespace Brawler
{
	class AssetCompiler
	{
	public:
		AssetCompiler();

		void BeginAssetCompilationPipeline(const AssetCompilerContext& context);

	private:
		void EnsureDirectoryValidity(const AssetCompilerContext& context) const;
		void CompileAssets(const AssetCompilerContext& context);

	private:
		BCALinker mBCALinker;
	};
}
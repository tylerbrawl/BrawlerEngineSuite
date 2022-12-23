module;
#include <compare>

module Brawler.PSOBuilderCreators;

namespace Brawler
{
	namespace PSOs
	{
		template <>
		PSOBuilder<Brawler::PSOID::BC7_TRY_MODE_456> CreatePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_456>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_456>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\BC7Encode.hlsl" },
				.EntryPoint{ L"TryMode456CS" },
				.MacroDefinitionArr{}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::BC7_TRY_MODE_137> CreatePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_137>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_137>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\BC7Encode.hlsl" },
				.EntryPoint{ L"TryMode137CS" },
				.MacroDefinitionArr{}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::BC7_TRY_MODE_02> CreatePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_02>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::BC7_TRY_MODE_02>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\BC7Encode.hlsl" },
				.EntryPoint{ L"TryMode02CS" },
				.MacroDefinitionArr{}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::BC7_ENCODE_BLOCK> CreatePSOBuilder<Brawler::PSOID::BC7_ENCODE_BLOCK>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::BC7_ENCODE_BLOCK>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\BC7Encode.hlsl" },
				.EntryPoint{ L"EncodeBlockCS" },
				.MacroDefinitionArr{}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE> CreatePSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\GenerateMipMaps.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE_SRGB> CreatePSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE_SRGB>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::GENERIC_DOWNSAMPLE_SRGB>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\GenerateMipMaps.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					// For tools like the Brawler Model Exporter, we might as well just use the exact sRGB
					// curve.
					{ L"__EXACT_SRGB__", L"1" },
					{ L"__USING_SRGB_DATA__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_POINT> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_POINT>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_POINT>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTextureTiling.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_POINT_FILTER__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_BILINEAR> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_BILINEAR>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_BILINEAR>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTextureTiling.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_BILINEAR_FILTER__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_TRILINEAR> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_TRILINEAR>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_TRILINEAR>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTextureTiling.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_TRILINEAR_FILTER__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_ANISOTROPIC> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_ANISOTROPIC>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_ANISOTROPIC>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTextureTiling.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_ANISOTROPIC_8X_FILTER__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_POINT_SRGB> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_POINT_SRGB>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_POINT_SRGB>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTextureTiling.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_POINT_FILTER__", L"1" },
					{ L"__USING_SRGB_DATA__", L"1" },
					{ L"__EXACT_SRGB__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_BILINEAR_SRGB> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_BILINEAR_SRGB>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_BILINEAR_SRGB>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTextureTiling.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_BILINEAR_FILTER__", L"1" },
					{ L"__USING_SRGB_DATA__", L"1" },
					{ L"__EXACT_SRGB__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_TRILINEAR_SRGB> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_TRILINEAR_SRGB>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_TRILINEAR_SRGB>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTextureTiling.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_TRILINEAR_FILTER__", L"1" },
					{ L"__USING_SRGB_DATA__", L"1" },
					{ L"__EXACT_SRGB__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_ANISOTROPIC_SRGB> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_ANISOTROPIC_SRGB>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_TILING_ANISOTROPIC_SRGB>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTextureTiling.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_ANISOTROPIC_8X_FILTER__", L"1" },
					{ L"__USING_SRGB_DATA__", L"1" },
					{ L"__EXACT_SRGB__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_POINT> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_POINT>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_POINT>(Brawler::ShaderCompilationParams{
				.FilePath{L"Shaders\\VirtualTexturePageMerging.hlsl"},
				.EntryPoint{L"main"},
				.MacroDefinitionArr{
					{L"__TILING_MODE_POINT_FILTER__", L"1"}
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_BILINEAR> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_BILINEAR>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_BILINEAR>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTexturePageMerging.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_BILINEAR_FILTER__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_TRILINEAR> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_TRILINEAR>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_TRILINEAR>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTexturePageMerging.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_TRILINEAR_FILTER__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_ANISOTROPIC> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_ANISOTROPIC>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_ANISOTROPIC>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTexturePageMerging.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_ANISOTROPIC_8X_FILTER__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_POINT_SRGB> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_POINT_SRGB>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_POINT_SRGB>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTexturePageMerging.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_POINT_FILTER__", L"1" },
					{ L"__USING_SRGB_DATA__", L"1" },
					{ L"__EXACT_SRGB__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_BILINEAR_SRGB> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_BILINEAR_SRGB>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_BILINEAR_SRGB>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTexturePageMerging.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_BILINEAR_FILTER__", L"1" },
					{ L"__USING_SRGB_DATA__", L"1" },
					{ L"__EXACT_SRGB__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_TRILINEAR_SRGB> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_TRILINEAR_SRGB>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_TRILINEAR_SRGB>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTexturePageMerging.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_TRILINEAR_FILTER__", L"1" },
					{ L"__USING_SRGB_DATA__", L"1" },
					{ L"__EXACT_SRGB__", L"1" }
				}
				});
		}

		template <>
		PSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_ANISOTROPIC_SRGB> CreatePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_ANISOTROPIC_SRGB>()
		{
			return CreateGeneralComputePSOBuilder<Brawler::PSOID::VIRTUAL_TEXTURE_PAGE_MERGING_ANISOTROPIC_SRGB>(Brawler::ShaderCompilationParams{
				.FilePath{ L"Shaders\\VirtualTexturePageMerging.hlsl" },
				.EntryPoint{ L"main" },
				.MacroDefinitionArr{
					{ L"__TILING_MODE_ANISOTROPIC_8X_FILTER__", L"1" },
					{ L"__USING_SRGB_DATA__", L"1" },
					{ L"__EXACT_SRGB__", L"1" }
				}
				});
		}
	}
}
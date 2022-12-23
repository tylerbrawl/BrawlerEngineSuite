module;
#include <utility>

export module Brawler.ShaderCompilationFlags;

export namespace Brawler
{
	enum class ShaderCompilationFlags
	{
		NONE = 0,

		/// <summary>
		/// Specifies that register mappings for a shader, despite being assigned different register
		/// and space IDs, may be mapped to the same region of a descriptor heap.
		/// 
		/// A prime example of this is bindless SRVs, where tons of different register mappings
		/// are made which all point to the bindless SRV segment of the GPUResourceDescriptorHeap.
		/// Any shaders which plan on using bindless SRVs should include this flag.
		/// </summary>
		RESOURCES_MAY_ALIAS = (1 << 0)
	};
}

export namespace Brawler
{
	constexpr ShaderCompilationFlags operator|(const ShaderCompilationFlags lhs, const ShaderCompilationFlags rhs)
	{
		return static_cast<ShaderCompilationFlags>(std::to_underlying(lhs) | std::to_underlying(rhs));
	}

	constexpr ShaderCompilationFlags& operator|=(ShaderCompilationFlags& lhs, const ShaderCompilationFlags rhs)
	{
		lhs = (lhs | rhs);
		return lhs;
	}

	constexpr ShaderCompilationFlags operator&(const ShaderCompilationFlags lhs, const ShaderCompilationFlags rhs)
	{
		return static_cast<ShaderCompilationFlags>(std::to_underlying(lhs) & std::to_underlying(rhs));
	}

	constexpr ShaderCompilationFlags& operator&=(ShaderCompilationFlags& lhs, const ShaderCompilationFlags rhs)
	{
		lhs = (lhs & rhs);
		return lhs;
	}

	constexpr ShaderCompilationFlags operator^(const ShaderCompilationFlags lhs, const ShaderCompilationFlags rhs)
	{
		return static_cast<ShaderCompilationFlags>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
	}

	constexpr ShaderCompilationFlags& operator^=(ShaderCompilationFlags& lhs, const ShaderCompilationFlags rhs)
	{
		lhs = (lhs ^ rhs);
		return lhs;
	}

	constexpr ShaderCompilationFlags operator~(const ShaderCompilationFlags operand)
	{
		return static_cast<ShaderCompilationFlags>(~(std::to_underlying(operand)));
	}
}
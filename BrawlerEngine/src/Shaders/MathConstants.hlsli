#pragma once

namespace BrawlerHLSL
{
	// We don't have constexpr in HLSL, and since floats are not integral types,
	// we can't use mathematical expressions like, e.g., (2.0f * PI) to define
	// TWO_PI without performing calculations at runtime.
	//
	// Thus, in HLSL code, we will use explicit values by default. (In C++ code,
	// we always prefer the use of constexpr, since it reduces the possibility
	// for mistakes and increases readability.)
	
	static const float PI = 3.14159265f;
	static const float INVERSE_PI = 0.31830989f;  // (1.0f / PI);
	static const float TWO_PI = 6.28318531f;  // (2.0f * PI);
}
module;

export module Brawler.ModelTextureID;

export namespace Brawler
{
	/// <summary>
	/// ModelTextureID values represent the different texture types which can be found within
	/// a material definition. Each material is free to use these types as they please, although
	/// the best practice would be to associate textures with their matching names.
	/// 
	/// As an example of the differences between types of material definitions, an opaque material
	/// definition would not need any information for transparency, so the alpha channel of its
	/// diffuse albedo texture could be used for some other purpose, such as a sub-surface scattering
	/// coefficient.
	/// </summary>
	enum class ModelTextureID
	{
		DIFFUSE_ALBEDO,

		COUNT_OR_ERROR
	};
}
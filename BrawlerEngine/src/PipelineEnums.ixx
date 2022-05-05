module;

export module Brawler.PipelineEnums;

export namespace Brawler
{
	/// <summary>
	/// PSOID is an enumeration of all of the pipeline state objects (PSOs) which the
	/// Brawler Engine recognizes. Each PSOID represents a unique PSO that can be used
	/// to set the pipeline state object of an ID3D12GraphicsCommandList (via an
	/// I_RenderContext instance).
	/// </summary>
	enum class PSOID
	{
		COUNT_OR_ERROR
	};

	enum class RootSignatureID
	{
		COUNT_OR_ERROR
	};
}
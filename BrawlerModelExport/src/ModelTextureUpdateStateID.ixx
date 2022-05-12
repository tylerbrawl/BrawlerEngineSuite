module;

export module Brawler.ModelTextureUpdateStateID;

export namespace Brawler
{
	enum class ModelTextureUpdateStateID
	{
		MIP_MAP_GENERATION_STATE,
		FORMAT_CONVERSION_STATE,
		AWAITING_SERIALIZATION_STATE,

		COUNT_OR_ERROR
	};
}
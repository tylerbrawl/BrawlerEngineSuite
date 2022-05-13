module;
#include <tuple>
#include <assimp/material.h>

export module Brawler.ModelTextureUpdateStateTraits;
import Brawler.PolymorphismInfo;
import Brawler.I_ModelTextureUpdateState;
import Brawler.MipMapGenerationModelTextureUpdateState;
import Brawler.FormatConversionModelTextureUpdateState;
import Brawler.AwaitingSerializationModelTextureUpdateState;
import Brawler.ModelTextureUpdateStateID;

namespace Brawler
{
	template <aiTextureType TextureType>
	using ModelTextureUpdateStateTuple = std::tuple<
		// ModelTextureUpdateStateID::MIP_MAP_GENERATION_STATE
		MipMapGenerationModelTextureUpdateState<TextureType>,

		// ModelTextureUpdateStateID::FORMAT_CONVERSION_STATE
		FormatConversionModelTextureUpdateState<TextureType>,

		// ModelTextureUpdateStateID::AWAITING_SERIALIZATION_STATE
		AwaitingSerializationModelTextureUpdateState<TextureType>
	>;
}

export namespace Brawler
{
	template <typename DummyType, aiTextureType TextureType>
	struct PolymorphismInfo<I_ModelTextureUpdateState<DummyType, TextureType>> : public PolymorphismInfoInstantiation<ModelTextureUpdateStateID, ModelTextureUpdateStateTuple<TextureType>>
	{};
}
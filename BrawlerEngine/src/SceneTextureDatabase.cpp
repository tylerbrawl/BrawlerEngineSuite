module;
#include <cstddef>
#include <tuple>

module Brawler.SceneTextureDatabase;

namespace Brawler
{
	SceneTextureDatabase& SceneTextureDatabase::GetInstance()
	{
		static SceneTextureDatabase instance{};
		return instance;
	}

	void SceneTextureDatabase::DeleteUnreferencedSceneTextures()
	{
		const auto deleteTexturesLambda = [this]<std::size_t CurrIndex>(this const auto& self)
		{
			if constexpr (CurrIndex != std::tuple_size_v<TypedDatabaseTuple_T>)
			{
				std::get<CurrIndex>(mDatabaseTuple).DeleteUnreferencedSceneTextures();

				static constexpr std::size_t NEXT_INDEX = (CurrIndex + 1);
				self.template operator()<NEXT_INDEX>();
			}
		};

		deleteTexturesLambda.template operator()<0>();
	}
}
module;

module Brawler.SceneTextureDatabase;

namespace Brawler
{
	SceneTextureDatabase& SceneTextureDatabase::GetInstance()
	{
		static SceneTextureDatabase instance{};
		return instance;
	}
}
module;

module Brawler.GameState;

namespace Brawler
{
	GameState::GameState(World&& world) :
		I_ApplicationState(),
		mWorld(std::move(world))
	{}

	bool GameState::Update(const float dt)
	{
		return false;
	}
}
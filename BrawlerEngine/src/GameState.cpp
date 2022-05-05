module;

module Brawler.GameState;
import Brawler.ApplicationStateStack;

namespace Brawler
{
	GameState::GameState(ApplicationStateStack& stateStack) :
		I_ApplicationState(stateStack)
	{}

	bool GameState::Update(const float dt)
	{
		return false;
	}
}
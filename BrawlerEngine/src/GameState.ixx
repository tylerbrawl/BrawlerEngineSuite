module;

export module Brawler.GameState;
import Brawler.I_ApplicationState;
import Brawler.World;

export namespace Brawler
{
	class ApplicationStateStack;
}

export namespace Brawler
{
	class GameState : public I_ApplicationState
	{
	public:
		explicit GameState(ApplicationStateStack& stateStack);

		bool Update(const float dt) override;

	private:
		World mWorld;
	};
}
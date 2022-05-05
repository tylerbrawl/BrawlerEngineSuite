module;

module Brawler.I_ApplicationState;

namespace Brawler
{
	I_ApplicationState::I_ApplicationState(ApplicationStateStack& stateStack) :
		mOwningStack(&stateStack)
	{}

	ApplicationStateStack& I_ApplicationState::GetStateStack()
	{
		return *mOwningStack;
	}

	const ApplicationStateStack& I_ApplicationState::GetStateStack() const
	{
		return *mOwningStack;
	}
}
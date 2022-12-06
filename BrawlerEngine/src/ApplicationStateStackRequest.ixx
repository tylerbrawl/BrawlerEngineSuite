module;
#include <memory>

export module Brawler.IMPL.ApplicationStateStackRequest;
import Brawler.ApplicationStateID;
import Brawler.I_ApplicationState;

export namespace Brawler
{
	namespace IMPL
	{
		enum class ApplicationStateStackRequestType
		{
			PUSH,
			POP,
			CLEAR
		};

		struct ApplicationStateStackRequest
		{
			ApplicationStateStackRequestType Type;
			std::unique_ptr<Brawler::I_ApplicationState> StatePtr;
		};
	}
}
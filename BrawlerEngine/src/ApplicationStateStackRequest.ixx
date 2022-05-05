module;

export module Brawler.IMPL.ApplicationStateStackRequest;
import Brawler.ApplicationStateID;

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
			Brawler::ApplicationStateID StateID;
		};
	}
}
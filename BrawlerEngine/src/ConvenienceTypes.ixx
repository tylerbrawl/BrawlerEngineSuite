module;

export module Brawler.ConvenienceTypes;

export namespace Brawler
{
	template <typename RetType, typename... Args>
	using FunctionPtr = RetType(*)(Args...);
}
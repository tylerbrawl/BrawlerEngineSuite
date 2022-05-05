module;
#include <functional>

export module Brawler.Functional;

export namespace Brawler
{
	template <typename T, typename RetType, typename... Args>
	concept Function = std::is_convertible_v<T, std::function<RetType(Args...)>>;
}
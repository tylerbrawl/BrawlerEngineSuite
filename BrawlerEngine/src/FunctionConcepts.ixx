module;
#include <functional>

export module Brawler.FunctionConcepts;

export namespace Brawler
{
	// As useful as std::function is, the fact that it does type erasure means that using it always
	// requires a virtual function call. If you want to pass a lambda function as a parameter to some
	// other function, but you do not plan on storing the lambda function, then you can use this
	// concept to avoid this.
	//
	// Brawler::Function is a concept which provides the type safety of std::function without the
	// overhead of a virtual function call. However, unlike std::function, it cannot be used to
	// store a lambda function.

	template <typename ObjType, typename RetType, typename... Args>
	concept Function = std::is_convertible_v<ObjType, std::function<RetType(Args...)>>;
}
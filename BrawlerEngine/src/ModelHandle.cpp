module;
#include <cassert>
#include <memory>

module Brawler.ModelHandle;

namespace Brawler
{
	ModelHandle::ModelHandle(std::shared_ptr<Model> sharedModelPtr) :
		mModelPtr(std::move(sharedModelPtr))
	{}

	const Model& ModelHandle::GetModel() const
	{
		assert(mModelPtr != nullptr);
		return *mModelPtr;
	}
}
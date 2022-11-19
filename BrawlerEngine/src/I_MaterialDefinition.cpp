module;
#include <atomic>

module Brawler.I_MaterialDefinition;

namespace Brawler
{
	void I_MaterialDefinition::NotifyForDeletion()
	{
		mReadyForDeletion.store(true, std::memory_order::relaxed);
	}

	bool I_MaterialDefinition::ReadyForDeletion() const
	{
		return mReadyForDeletion.load(std::memory_order::relaxed);
	}
}
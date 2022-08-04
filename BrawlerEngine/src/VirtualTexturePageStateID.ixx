module;

export module Brawler.VirtualTexturePageStateID;

export namespace Brawler
{
	enum class VirtualTexturePageStateID
	{
		PAGE_NEUTRAL_STATE,
		PAGE_ADDITION_STATE,
		PAGE_PENDING_REMOVAL_STATE,
		PAGE_COMMITTED_REMOVAL_STATE,
		PAGE_TRANSFER_STATE,

		COUNT_OR_ERROR
	};
}
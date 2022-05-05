module;

export module Brawler.D3D12.GPUResourceSpecialInitializationMethod;

export namespace Brawler
{
	namespace D3D12
	{
		enum class GPUResourceSpecialInitializationMethod
		{
			/// <summary>
			/// This method correlates to a call to ID3D12GraphicsCommandList::DiscardResource().
			/// 
			/// Use this method when you do not care about the contents of an aliased resource
			/// before using it. For example, if you are going to draw over an entire render
			/// target anyways, you may as well just discard the results with this method.
			/// </summary>
			DISCARD,

			/// <summary>
			/// This method correlates to a call to either ID3D12GraphicsCommandList::ClearRenderTargetView()
			/// or ID3D12GraphicsCommandList::ClearDepthStencilView(), depending on the type
			/// of resource being initialized.
			/// 
			/// The actual clear value for a resource will be that which is returned by the
			/// (derived) I_GPUResource::GetOptimizedClearValue() function. An assert is fired
			/// in Debug builds if this function returns an empty std::optional instance
			/// and the resource chooses this method for alias initialization.
			/// 
			/// Use this method when you need the contents of the resource to be in a specific
			/// state before using it. For example, the contents of a depth/stencil buffer
			/// typically need to be set to a specific value before, say, a Z-pre-pass.
			/// </summary>
			CLEAR
		};
	}
}
module;

export module Brawler.D3D12.I_RenderModule;

export namespace Brawler
{
	namespace D3D12
	{
		class FrameGraphBuilder;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class I_RenderModule
		{
		protected:
			I_RenderModule() = default;

		public:
			virtual ~I_RenderModule() = default;

			I_RenderModule(const I_RenderModule& rhs) = delete;
			I_RenderModule& operator=(const I_RenderModule& rhs) = delete;

			I_RenderModule(I_RenderModule&& rhs) noexcept = default;
			I_RenderModule& operator=(I_RenderModule&& rhs) noexcept = default;

			/// <summary>
			/// This function is used to determine if an I_RenderModule instance's commands
			/// will be recorded for the current frame.
			/// 
			/// If an I_RenderModule instance should not always have its commands be submitted to
			/// the GPU, then derived classes can override this function to declare when they should
			/// be.
			/// </summary>
			/// <returns>
			/// The function returns true if the I_RenderModule instance should have its commands
			/// recorded for this frame and false otherwise. By default, this function always returns
			/// true.
			/// </returns>
			virtual bool IsRenderModuleEnabled() const;

			FrameGraphBuilder CreateFrameGraphBuilder();

		protected:
			virtual void BuildFrameGraph(FrameGraphBuilder& builder) = 0;

		private:
			void FinalizeFrameGraphBuilder(FrameGraphBuilder& builder) const;
		};
	}
}
module;
#include <DxDef.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.I_ViewComponent;
import Brawler.I_Component;
import Brawler.ComponentID;

export namespace Brawler
{
	class I_ViewComponent : public I_Component
	{
	public:
		/// <summary>
		/// In the Brawler Engine, a view is responsible for creating render jobs,
		/// much like in the Destiny Engine. The ViewType enumeration specifies which
		/// type of view a given I_ViewComponent represents.
		/// 
		/// It is not feasible to execute render jobs for every view which exists in
		/// a scene graph. Thus, as an optimization, we specify the type of view to
		/// describe under what circumstances a given view should be allowed to
		/// create render jobs.
		/// 
		/// The exact details of when a view creates render jobs is specified in the
		/// documentation for each individual value of the ViewType enumeration.
		/// </summary>
		enum class ViewType
		{
			/// <summary>
			/// These views will always create render jobs. The player's camera is a
			/// great example of this.
			/// </summary>
			PRIMARY,

			/// <summary>
			/// These views will only create render jobs if their bounding volume is
			/// within the view frustum of a ViewType::PRIMARY view. To save on
			/// performance, these views cannot trigger other ViewType::SECONDARY
			/// views.
			/// 
			/// Environment probes and planar reflection views are good examples
			/// of this type.
			/// </summary>
			SECONDARY,

			/// <summary>
			/// These views will only create render jobs if their bounding volume is
			/// within the view frustum of either a ViewType::PRIMARY view or a
			/// ViewType::SECONDARY view which was activated by a ViewType::PRIMARY
			/// view.
			/// 
			/// Light views for shadow mapping are good examples of this type.
			/// </summary>
			TERTIARY,

			COUNT_OR_ERROR
		};

	protected:
		explicit I_ViewComponent(const ViewType type);

	public:
		virtual ~I_ViewComponent() = default;

		ViewType GetViewType() const;

		/// <summary>
		/// Determines whether or not this view has a reversed depth buffer, i.e.,
		/// its near and far planes are swapped. By default, this is enabled.
		/// </summary>
		/// <returns>
		/// The function returns true if this view has a reversed depth buffer and
		/// false otherwise. The default return value is true, but derived classes
		/// can override this behavior.
		/// </returns>
		virtual bool IsDepthBufferReversed() const;

	private:
		ViewType mType;
		DirectX::XMFLOAT4X4 mViewMatrix;
		DirectX::XMFLOAT4X4 mViewProjMatrix;
	};
}
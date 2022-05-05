module;
#include "DxDef.h"

module Brawler.I_ViewComponent;

namespace Brawler
{
	I_ViewComponent::I_ViewComponent(SceneNode& owningNode, const ViewType type) :
		I_Component(owningNode),
		mType(type),
		mViewMatrix(),
		mViewProjMatrix()
	{}

	bool I_ViewComponent::IsDepthBufferReversed() const
	{
		return true;
	}
}
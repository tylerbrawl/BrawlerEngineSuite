module;

module Brawler.Camera;
import Brawler.TransformComponent;
import Brawler.ViewComponent;

namespace Brawler
{
	Camera::Camera() :
		SceneNode()
	{
		// Every Camera instance should have a TransformComponent and a ViewComponent.
		CreateComponent<TransformComponent>();
		CreateComponent<ViewComponent>();

		// And... that's it, for now. Any specific view parameters are instead tuned
		// through the ViewComponent instance. We could expose these as convenience
		// member functions, but I don't really think that's necessary.
	}
}
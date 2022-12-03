module;

export module Brawler.Camera;
import Brawler.SceneNode;

export namespace Brawler
{
	class Camera final : public SceneNode
	{
	public:
		Camera();

		Camera(const Camera& rhs) = delete;
		Camera& operator=(const Camera& rhs) = delete;

		Camera(Camera&& rhs) noexcept = default;
		Camera& operator=(Camera&& rhs) noexcept = default;
	};
}
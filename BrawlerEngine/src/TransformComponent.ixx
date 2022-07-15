module;

export module Brawler.TransformComponent;
import Brawler.I_Component;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	class TransformComponent final : public I_Component
	{
	private:
		Math::Float3 mScale;
		
	};
}
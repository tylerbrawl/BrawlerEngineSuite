module;
#include <DirectXMath/DirectXMath.h>

export module Brawler.NormalBoundingCone;

export namespace Brawler
{
	struct NormalBoundingCone
	{
		DirectX::XMFLOAT3 ConeNormal;
		float NegativeSineAngle;
	};
}
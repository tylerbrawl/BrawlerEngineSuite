module;
#include <utility>
#include <DirectXMath/DirectXMath.h>

export module Brawler.Math.MathTypes:Quaternion;
import :Matrix;
import :Vector;
import Util.Math;

export namespace Brawler
{
	namespace Math
	{
		class Quaternion
		{
		public:
			constexpr Quaternion() = default;
			constexpr explicit Quaternion(const Float3& normalizedRotationAxis, const float rotationInRadians);
			constexpr explicit Quaternion(const DirectX::XMFLOAT4& quaternionData);
			constexpr explicit Quaternion(const Float3x3& rotationMatrix);

			constexpr Quaternion(const Quaternion& rhs) = default;
			constexpr Quaternion& operator=(const Quaternion& rhs) = default;

			constexpr Quaternion(Quaternion&& rhs) noexcept = default;
			constexpr Quaternion& operator=(Quaternion&& rhs) noexcept = default;

			constexpr Quaternion Normalize() const;
			constexpr float GetMagnitude() const;

			constexpr float GetMagnitudeSquared() const;

			/// <summary>
			/// Performs a "true" multiplication between two Quaternion instances. Specifically, let
			/// the quaternion p represent this Quaternion instance, and let q represent rhs. Then,
			/// this function returns the Quaternion pq. If both p and q are normalized, then the
			/// returned Quaternion instance will also be normalized.
			/// 
			/// This is *NOT* the same as calling DirectX::XMQuaternionMultiply(p, q)! That function
			/// returns the result of qp with the assumption that it is being called to concatenate
			/// two rotation Quaternions. If this is the desired result, then one should call
			/// p.ChainRotation(q) instead of p.MultiplyQuaternion(q).
			/// </summary>
			/// <param name="rhs">
			/// - The Quaternion instance which will represent the right-hand side of the Quaternion
			///   multiplication operation.
			/// </param>
			/// <returns>
			/// Let the quaternion p represent this Quaternion instance, and let q represent rhs. Then,
			/// this function returns the Quaternion pq.
			/// </returns>
			constexpr Quaternion MultiplyQuaternion(const Quaternion& rhs) const;

			constexpr Quaternion MultiplyScalar(const float rhs) const;
			constexpr Quaternion DivideScalar(const float rhs) const;

			/// <summary>
			/// Assuming that both this Quaternion instance and nextRotationQuaternion are both
			/// unit quaternions, this function calculates and returns the rotation created by
			/// first rotating by this Quaternion instance and then rotating by nextRotationQuaternion.
			/// 
			/// Mathematically, let p and q be two unit quaternions. Then, this function will
			/// return the result of qp, which represents a rotation by p followed by a rotation
			/// by q. Importantly, the quaternion qp will also be a unit quaternion; this allows
			/// for seamlessly chaining rotation quaternions.
			/// 
			/// Notably, calling p.ChainRotation(q) will return the same result as
			/// DirectX::XMQuaternionMultiply(p, q), because the DirectXMath function computes the
			/// result of that call as qp (i.e., it swaps the operands). For a "true" multiplication
			/// between Quaternions p and q, call p.MultiplyQuaternion(q).
			/// 
			/// *NOTE*: The function asserts in Debug builds if either this Quaternion instance
			/// or nextRotationQuaternion is unnormalized.
			/// </summary>
			/// <param name="nextRotationQuaternion">
			/// - The unit Quaternion instance which represents the next rotation in the chain.
			/// </param>
			/// <returns>
			/// Let p represent this Quaternion instance and let q represent nextRotationQuaternion.
			/// Then, assuming that both p and q are normalized, this function returns the unit/normalized
			/// Quaternion qp.
			/// </returns>
			constexpr Quaternion ChainRotation(const Quaternion& nextRotationQuaternion) const;

			/// <summary>
			/// Assuming that this Quaternion instance is a unit quaternion, this function returns the
			/// 3x3 matrix representing the corresponding rotation. The matrix can be used in the creation
			/// of world matrices, among other things.
			/// 
			/// Mathematically, the returned matrix is guaranteed to be an orthogonal matrix. This has
			/// some useful properties. For instance, it means that both its rows and columns form an
			/// orthonormal basis, regardless of whether row vectors or column vectors are used. This implies
			/// that a Quaternion can be used to represent any three-dimensional space given three linearly-
			/// independent vectors which form its basis. View space, tangent space, you name it! In addition,
			/// the inverse of an orthogonal matrix M is equal to its transpose; that is,
			/// (Inverse(M) == Transpose(M)).
			/// 
			/// Considering this, it may appear that "ConvertToRotationMatrix()" is not the best name for
			/// this function at first glance. As it turns out, however, every rotation is an orthogonal
			/// matrix, and every orthogonal matrix leads to a unique rotation. Why the hell isn't this
			/// talked about more often?!
			/// 
			/// *NOTE*: The function asserts in Debug builds if this Quaternion instance is unnormalized.
			/// </summary>
			/// <returns>
			/// The function returns the 3x3 matrix representing the rotation represented by this Quaternion
			/// instance.
			/// </returns>
			constexpr Float3x3 ConvertToRotationMatrix() const;

			constexpr Quaternion Inverse() const;
			constexpr Quaternion Conjugate() const;

			constexpr bool IsNormalized() const;

			constexpr Quaternion GetReciprocal() const;

			constexpr float GetScalarComponent() const;
			constexpr Float3 GetVectorComponent() const;
			
			DirectX::XMVECTOR GetDirectXMathVector() const;

			constexpr Quaternion& operator*=(const Quaternion& rhs);

			constexpr Quaternion& operator*=(const float rhs);
			constexpr Quaternion& operator/=(const float rhs);

		private:
			/// <summary>
			/// DirectXMath stores its quaternions as a four-dimensional vector [x y z w],
			/// where [x y z] is the vector component of the quaternion and w is the scalar
			/// component. We do the same.
			/// </summary>
			DirectX::XMFLOAT4 mQuaternionVector;
		};
	}
}

export
{
	template <typename T>
		requires std::is_arithmetic_v<T>
	constexpr Brawler::Math::Quaternion operator*(const Brawler::Math::Quaternion& lhs, const T rhs);

	template <typename T>
		requires std::is_arithmetic_v<T>
	constexpr Brawler::Math::Quaternion operator*(const T lhs, const Brawler::Math::Quaternion& rhs);

	template <typename T>
		requires std::is_arithmetic_v<T>
	constexpr Brawler::Math::Quaternion operator/(const Brawler::Math::Quaternion& lhs, const T rhs);

	template <typename T>
		requires std::is_arithmetic_v<T>
	constexpr Brawler::Math::Quaternion operator/(const T lhs, const Brawler::Math::Quaternion& rhs);

	constexpr Brawler::Math::Quaternion operator*(const Brawler::Math::Quaternion& lhs, const Brawler::Math::Quaternion& rhs);
}

// -------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace Math
	{
		constexpr Quaternion::Quaternion(const Float3& normalizedRotationAxis, const float rotationInRadians) :
			mQuaternionVector()
		{
			assert(normalizedRotationAxis.IsNormalized() && "ERROR: An attempt was made to construct a Quaternion from an unnormalized rotation axis vector!");

			if (std::is_constant_evaluated())
			{
				// We don't have constexpr for <cmath> (yet!), so we need to write our own approximation
				// using the power series definition to converge to the correct result. We'll allow long-running
				// calculations because this code path is only taken at compile time.

				const float halfAngle = (rotationInRadians / 2.0f);

				constexpr auto FACTORIAL_LAMBDA = [] (const std::size_t startingValue)
				{
					std::size_t currFactorialValue = 1;

					for (std::size_t currValue = startingValue; currValue > 1; --currValue)
						currFactorialValue *= currValue;

					return currFactorialValue;
				};

				constexpr float MAXIMUM_ALLOWED_DIFFERENCE = 0.001f;

				float currSineValue = 0.0f;
				float prevSineValue = currSineValue;
				std::uint32_t currIteration = 0;

				const std::int32_t halfAngleFloatExponent = ((std::bit_cast<std::int32_t>(halfAngle) >> 23) & 0xFF) - 127;
				const float halfAngleNoExponent = std::bit_cast<float>(std::bit_cast<std::int32_t>(halfAngle) & 0x807FFFFF);

				while (true)
				{
					const std::uint32_t iterationExponentValue = ((2 * currIteration) + 1);

					// Get the value of halfAngle^(currExponentValue). By exploiting the layout of IEEE-754
					// floating-point values, we can calculate this in constant time.
					const std::uint32_t newHalfAngleFloatExponent = (halfAngleFloatExponent * iterationExponentValue) + 127;

					float currScaledHalfAngle = std::bit_cast<float>(std::bit_cast<std::int32_t>(halfAngleNoExponent) | (newHalfAngleFloatExponent << 23));
					currScaledHalfAngle *= (currIteration % 2 == 0 ? 1.0f : -1.0f);
					currScaledHalfAngle /= FACTORIAL_LAMBDA(iterationExponentValue);

					currSineValue += currScaledHalfAngle;
					
					float sineValueDifference = (currSineValue - prevSineValue);

					if (sineValueDifference < 0.0f)
						sineValueDifference *= -1.0f;

					if (sineValueDifference < MAXIMUM_ALLOWED_DIFFERENCE)
						break;

					prevSineValue = currSineValue;
					++currIteration;
				}

				// Now, currSineValue ~= sin(halfAngle). We need this to get the quaternion, but we also need cos(halfAngle).
				// However, we know that (sin(halfAngle))^2 + (cos(halfAngle))^2 = 1, so we can solve for that without
				// using the power series definition for cosine.
				const float sineHalfAngle = currSineValue;
				const float cosineHalfAngle = Util::Math::GetSquareRoot(1.0f - (sineHalfAngle * sineHalfAngle));

				// Finally, we can store the quaternion as q = (sineHalfAngle * normalizedRotationAxis, cosineHalfAngle).
				const Float3 scaledRotationNormal{ normalizedRotationAxis * sineHalfAngle };

				mQuaternionVector.x = scaledRotationNormal.GetX();
				mQuaternionVector.y = scaledRotationNormal.GetY();
				mQuaternionVector.z = scaledRotationNormal.GetZ();
				mQuaternionVector.w = cosineHalfAngle;
			}
			else
			{
				const DirectX::XMVECTOR loadedNormalAxis{ normalizedRotationAxis.GetDirectXMathVector() };
				DirectX::XMStoreFloat4(&mQuaternionVector, DirectX::XMQuaternionRotationNormal(loadedNormalAxis, rotationInRadians));
			}
		}

		constexpr Quaternion::Quaternion(const DirectX::XMFLOAT4& quaternionData) :
			mQuaternionVector(quaternionData)
		{}

		constexpr Quaternion::Quaternion(const Float3x3& rotationMatrix) :
			mQuaternionVector()
		{
			if (std::is_constant_evaluated())
			{
				// These equations were derived by hand. There are equivalent expressions which involve less
				// square roots, but these equations will always produce defined results because a divide
				// by zero never happens.
				//
				// In addition, this code path is only taken at compile time, so we don't pay for this at
				// runtime.

				const float quaternionYSquared = ((rotationMatrix.GetElement(1, 1) - rotationMatrix.GetElement(0, 0) - rotationMatrix.GetElement(2, 2) + 1.0f) / 4.0f);
				mQuaternionVector.y = Util::Math::GetSquareRoot(quaternionYSquared);

				const float twoTimesQuaternionYSquared = (2.0f * quaternionYSquared);

				const float quaternionXSquared = ((rotationMatrix.GetElement(0, 0) + twoTimesQuaternionYSquared - rotationMatrix.GetElement(1, 1)) / 2.0f);
				mQuaternionVector.x = Util::Math::GetSquareRoot(quaternionXSquared);

				const float quaternionZSquared = ((1.0f - rotationMatrix.GetElement(0, 0) - twoTimesQuaternionYSquared) / 2);
				mQuaternionVector.z = Util::Math::GetSquareRoot(quaternionZSquared);

				// Construct a unit quaternion, meaning that the sum of all squared components should equal
				// one. So, if x^2 + y^2 + z^2 + w^2 = 1, then w^2 = 1 - x^2 - y^2 - z^2.
				mQuaternionVector.w = Util::Math::GetSquareRoot(1.0f - quaternionXSquared - quaternionYSquared - quaternionZSquared);
			}
			else
			{
				const DirectX::XMMATRIX loadedRotationMatrix{ rotationMatrix.GetDirectXMathMatrix() };
				const DirectX::XMVECTOR loadedQuatVector{ DirectX::XMQuaternionRotationMatrix(loadedRotationMatrix) };

				DirectX::XMStoreFloat4(&mQuaternionVector, loadedQuatVector);
			}
		}

		constexpr Quaternion Quaternion::Normalize() const
		{
			if (std::is_constant_evaluated())
			{
				const float magnitude = GetMagnitude();

				// Avoid dividing by zero.
				{
					constexpr float EPSILON = 0.00001f;

					// We don't need abs(magnitude) because magnitude > 0 in all cases.
					if (magnitude < EPSILON) [[unlikely]]
						return Quaternion{};
				}
				
				const DirectX::XMFLOAT4 normalizedResult{
					(mQuaternionVector.x / magnitude),
					(mQuaternionVector.y / magnitude),
					(mQuaternionVector.z / magnitude),
					(mQuaternionVector.w / magnitude)
				};
				return Quaternion{ normalizedResult };
			}
			else
			{
				const DirectX::XMVECTOR loadedThis{ DirectX::XMLoadFloat4(&mQuaternionVector) };

				const DirectX::XMVECTOR normalizedThis{ DirectX::XMQuaternionNormalize(loadedThis) };

				DirectX::XMFLOAT4 storedResult{};
				DirectX::XMStoreFloat4(&storedResult, normalizedThis);

				return Quaternion{ storedResult };
			}
		}

		constexpr float Quaternion::GetMagnitude() const
		{
			if (std::is_constant_evaluated())
			{
				const float squaredComponentSum = GetMagnitudeSquared();
				return Util::Math::GetSquareRoot(squaredComponentSum);
			}
			else
			{
				const DirectX::XMVECTOR loadedThis{ DirectX::XMLoadFloat4(&mQuaternionVector) };

				const DirectX::XMVECTOR magnitudeResult{ DirectX::XMQuaternionLength(loadedThis) };
				return DirectX::XMVectorGetX(magnitudeResult);
			}
		}

		constexpr float Quaternion::GetMagnitudeSquared() const
		{
			if (std::is_constant_evaluated())
			{
				return (mQuaternionVector.x * mQuaternionVector.x) + (mQuaternionVector.y * mQuaternionVector.y) + (mQuaternionVector.z * mQuaternionVector.z) +
					(mQuaternionVector.w * mQuaternionVector.w);
			}
			else
			{
				const DirectX::XMVECTOR loadedThis{ DirectX::XMLoadFloat4(&mQuaternionVector) };

				const DirectX::XMVECTOR magnitudeSquaredResult{ DirectX::XMQuaternionLengthSq(loadedThis) };
				return DirectX::XMVectorGetX(magnitudeSquaredResult);
			}
		}

		constexpr Quaternion Quaternion::MultiplyQuaternion(const Quaternion& rhs) const
		{
			if (std::is_constant_evaluated())
			{
				// Quaternion multiplication is defined as follows:
				//
				// pq = (v1, s1)(v2, s2) = (s1v2 + s2v1 + (v1 x v2), s1s2 - (v1 . v2)
				//
				// where s1 and s2 are the scalar components of the quaternions p and q,
				// respectively, and v1 and v2 are the vector components of the quaternions
				// p and q, respectively. Also, (v1 x v2) represents the cross product, while
				// (v1 . v2) represents the dot product.

				const Float3 v1 = Float3{ DirectX::XMFLOAT3{ mQuaternionVector.x, mQuaternionVector.y, mQuaternionVector.z } };
				const float s1 = mQuaternionVector.w;

				const Float3 v2 = Float3{ DirectX::XMFLOAT3{ rhs.mQuaternionVector.x, rhs.mQuaternionVector.y, rhs.mQuaternionVector.z } };
				const float s2 = rhs.mQuaternionVector.w;

				const Float3 resultVectorComponent{ (s1 * v2) + (s2 * v1) + v1.Cross(v2) };
				const float resultScalarComponent = (s1 * s2) - v1.Dot(v2);

				const DirectX::XMFLOAT4 storedResult{
					resultVectorComponent.GetX(),
					resultVectorComponent.GetY(),
					resultVectorComponent.GetZ(),
					resultScalarComponent
				};

				return Quaternion{ storedResult };
			}
			else
			{
				// DirectXMath::XMQuaternionMultiply(Q1, Q2) actually calculates Q2 * Q1, which is
				// the opposite of what we want here. Quaternion::MultiplyQuaternion() is meant to
				// represent an actual quaternion multiplication. We can still use the DirectXMath
				// function, but we have to swap the operands.
				const DirectX::XMVECTOR loadedLHS{ DirectX::XMLoadFloat4(&(rhs.mQuaternionVector)) };
				const DirectX::XMVECTOR loadedRHS{ DirectX::XMLoadFloat4(&mQuaternionVector) };

				const DirectX::XMVECTOR multiplyResult{ DirectX::XMQuaternionMultiply(loadedLHS, loadedRHS) };

				DirectX::XMFLOAT4 storedResult{};
				DirectX::XMStoreFloat4(&storedResult, multiplyResult);

				return Quaternion{ storedResult };
			}
		}

		constexpr Quaternion Quaternion::MultiplyScalar(const float rhs) const
		{
			if (std::is_constant_evaluated())
			{
				const DirectX::XMFLOAT4 scaledResult{
					(mQuaternionVector.x * rhs),
					(mQuaternionVector.y * rhs),
					(mQuaternionVector.z * rhs),
					(mQuaternionVector.w * rhs)
				};

				return Quaternion{ scaledResult };
			}
			else
			{
				const DirectX::XMVECTOR loadedThis{ DirectX::XMLoadFloat4(&mQuaternionVector) };

				const DirectX::XMVECTOR scaledVector{ DirectX::XMVectorScale(loadedThis, rhs) };

				DirectX::XMFLOAT4 storedResult{};
				DirectX::XMStoreFloat4(&storedResult, scaledVector);

				return Quaternion{ storedResult };
			}
		}

		constexpr Quaternion Quaternion::DivideScalar(const float rhs) const
		{
			if (std::is_constant_evaluated())
			{
				const DirectX::XMFLOAT4 divideResult{
					(mQuaternionVector.x / rhs),
					(mQuaternionVector.y / rhs),
					(mQuaternionVector.z / rhs),
					(mQuaternionVector.w / rhs)
				};

				return Quaternion{ divideResult };
			}
			else
			{
				const DirectX::XMVECTOR loadedThis{ DirectX::XMLoadFloat4(&mQuaternionVector) };

				const DirectX::XMVECTOR scaledVector{ DirectX::XMVectorScale(loadedThis, (1.0f / rhs)) };

				DirectX::XMFLOAT4 storedResult{};
				DirectX::XMStoreFloat4(&storedResult, scaledVector);

				return Quaternion{ storedResult };
			}
		}

		constexpr Quaternion Quaternion::ChainRotation(const Quaternion& nextRotationQuaternion) const
		{
			// Consider two unit quaternions, p and q, both of which represent rotations. Then,
			// a quaternion r which represents a rotation specified by p followed by a rotation
			// specified by q is defined as r = qp.

			return nextRotationQuaternion.MultiplyQuaternion(*this);
		}

		constexpr Float3x3 Quaternion::ConvertToRotationMatrix() const
		{
			if (std::is_constant_evaluated())
			{
				const float quaternionXSquared = (mQuaternionVector.x * mQuaternionVector.x);
				const float quaternionYSquared = (mQuaternionVector.y * mQuaternionVector.y);
				const float quaternionZSquared = (mQuaternionVector.z * mQuaternionVector.z);
				const float quaternionWSquared = (mQuaternionVector.w * mQuaternionVector.w);

				const float twoTimesXY = (2.0f * mQuaternionVector.x * mQuaternionVector.y);
				const float twoTimesZW = (2.0f * mQuaternionVector.z * mQuaternionVector.w);

				const float twoTimesXZ = (2.0f * mQuaternionVector.x * mQuaternionVector.z);
				const float twoTimesYW = (2.0f * mQuaternionVector.y * mQuaternionVector.w);

				const float twoTimesYZ = (2.0f * mQuaternionVector.y * mQuaternionVector.z);
				const float twoTimesXW = (2.0f * mQuaternionVector.x * mQuaternionVector.w);

				// I'll leave the more readable version below:
				//rotationMatrixStorage_11 = (1.0f - (2.0f * quaternionYSquared) - (2.0f * quaternionZSquared));
				//rotationMatrixStorage._12 = (twoTimesXY + twoTimesZW);
				//rotationMatrixStorage._13 = (twoTimesXZ - twoTimesYW);

				//rotationMatrixStorage._21 = (twoTimesXY - twoTimesZW);
				//rotationMatrixStorage._22 = (1.0f - (2.0f * quaternionXSquared) - (2.0f * quaternionZSquared));
				//rotationMatrixStorage._23 = (twoTimesYZ + twoTimesXW);

				//rotationMatrixStorage._31 = (twoTimesXZ + twoTimesYW);
				//rotationMatrixStorage._32 = (twoTimesYZ - twoTimesXW);
				//rotationMatrixStorage._33 = (1.0f - (2.0f * quaternionXSquared) - (2.0f * quaternionYSquared));

				return Float3x3{ 
					(1.0f - (2.0f * quaternionYSquared) - (2.0f * quaternionZSquared)), (twoTimesXY + twoTimesZW), (twoTimesXZ - twoTimesYW),
					(twoTimesXY - twoTimesZW), (1.0f - (2.0f * quaternionXSquared) - (2.0f * quaternionZSquared)), (twoTimesYZ + twoTimesXW),
					(twoTimesXZ + twoTimesYW), (twoTimesYZ - twoTimesXW), (1.0f - (2.0f * quaternionXSquared) - (2.0f * quaternionYSquared))
				};
			}
			else
			{
				const DirectX::XMVECTOR loadedThis{ DirectX::XMLoadFloat4(&mQuaternionVector) };

				const DirectX::XMMATRIX loadedRotationMatrix{ DirectX::XMMatrixRotationQuaternion(loadedThis) };

				DirectX::XMFLOAT3X3 storedResult{};
				DirectX::XMStoreFloat3x3(&storedResult, loadedRotationMatrix);

				return Float3x3{
					storedResult(0, 0), storedResult(0, 1), storedResult(0, 2),
					storedResult(1, 0), storedResult(1, 1), storedResult(1, 2),
					storedResult(2, 0), storedResult(2, 1), storedResult(2, 2)
				};
			}
		}

		constexpr Quaternion Quaternion::Inverse() const
		{
			if (std::is_constant_evaluated())
			{
				// The inverse of a quaternion q, q^(-1), is defined as follows:
				//
				// q^(-1) = q* / ||q||^2
				//
				// where q* denotes the conjugate of the quaternion q and ||q|| denotes
				// its magnitude.

				const Quaternion conjugateQuaternion{ Conjugate() };
				const float magnitudeSquared = GetMagnitudeSquared();

				return conjugateQuaternion / magnitudeSquared;
			}
			else
			{
				const DirectX::XMVECTOR loadedThis{ DirectX::XMLoadFloat4(&mQuaternionVector) };

				const DirectX::XMVECTOR inverseResult{ DirectX::XMQuaternionInverse(loadedThis) };

				DirectX::XMFLOAT4 storedResult{};
				DirectX::XMStoreFloat4(&storedResult, inverseResult);

				return Quaternion{ storedResult };
			}
		}

		constexpr Quaternion Quaternion::Conjugate() const
		{
			if (std::is_constant_evaluated())
			{
				DirectX::XMFLOAT4 storedConjugate{
					(mQuaternionVector.x * -1.0f),
					(mQuaternionVector.y * -1.0f),
					(mQuaternionVector.z * -1.0f),
					mQuaternionVector.w
				};

				return Quaternion{ storedConjugate };
			}
			else
			{
				const DirectX::XMVECTOR loadedThis{ DirectX::XMLoadFloat4(&mQuaternionVector) };

				const DirectX::XMVECTOR loadedConjugate{ DirectX::XMQuaternionConjugate(loadedThis) };

				DirectX::XMFLOAT4 storedResult{};
				DirectX::XMStoreFloat4(&storedResult, loadedConjugate);

				return Quaternion{ storedResult };
			}
		}

		constexpr bool Quaternion::IsNormalized() const
		{
			if (std::is_constant_evaluated())
			{
				const float squaredComponentSum = (mQuaternionVector.x * mQuaternionVector.x) + (mQuaternionVector.y * mQuaternionVector.y) + (mQuaternionVector.z * mQuaternionVector.z) +
					(mQuaternionVector.w * mQuaternionVector.w);

				constexpr float EPSILON = 0.00001f;
				float sumDifference = (squaredComponentSum - 1.0f);

				if (sumDifference < 0.0f)
					sumDifference *= -1.0f;

				return (sumDifference < EPSILON);
			}
			else
			{
				const DirectX::XMVECTOR loadedThis{ DirectX::XMLoadFloat4(&mQuaternionVector) };

				const DirectX::XMVECTOR lengthSquaredVector{ DirectX::XMQuaternionLengthSq(loadedThis) };
				const DirectX::XMVECTOR nearOneResultsVector{ DirectX::XMVectorNearEqual(lengthSquaredVector, DirectX::XMVectorReplicate(1.0f), DirectX::XMVectorSplatEpsilon()) };

				return (DirectX::XMVectorGetIntX(nearOneResultsVector) != 0);
			}
		}

		constexpr Quaternion Quaternion::GetReciprocal() const
		{
			if (std::is_constant_evaluated())
			{
				const DirectX::XMFLOAT4 storedReciprocal{
					(1.0f / mQuaternionVector.x),
					(1.0f / mQuaternionVector.y),
					(1.0f / mQuaternionVector.z),
					(1.0f / mQuaternionVector.w)
				};

				return Quaternion{ storedReciprocal };
			}
			else
			{
				const DirectX::XMVECTOR loadedThis{ DirectX::XMLoadFloat4(&mQuaternionVector) };

				const DirectX::XMVECTOR loadedReciprocal{ DirectX::XMVectorReciprocal(loadedThis) };

				DirectX::XMFLOAT4 storedResult{};
				DirectX::XMStoreFloat4(&storedResult, loadedReciprocal);

				return Quaternion{ storedResult };
			}
		}

		constexpr float Quaternion::GetScalarComponent() const
		{
			return mQuaternionVector.w;
		}

		constexpr Float3 Quaternion::GetVectorComponent() const
		{
			return Float3{ DirectX::XMFLOAT3{
				mQuaternionVector.x,
				mQuaternionVector.y,
				mQuaternionVector.z
			} };
		}

		DirectX::XMVECTOR Quaternion::GetDirectXMathVector() const
		{
			return DirectX::XMLoadFloat4(&mQuaternionVector);
		}

		constexpr Quaternion& Quaternion::operator*=(const Quaternion& rhs)
		{
			*this = MultiplyQuaternion(rhs);
			return *this;
		}

		constexpr Quaternion& Quaternion::operator*=(const float rhs)
		{
			*this = MultiplyScalar(rhs);
			return *this;
		}

		constexpr Quaternion& Quaternion::operator/=(const float rhs)
		{
			*this = DivideScalar(rhs);
			return *this;
		}
	}
}

template <typename T>
	requires std::is_arithmetic_v<T>
constexpr Brawler::Math::Quaternion operator*(const Brawler::Math::Quaternion& lhs, const T rhs)
{
	return lhs.MultiplyScalar(static_cast<float>(rhs));
}

template <typename T>
	requires std::is_arithmetic_v<T>
constexpr Brawler::Math::Quaternion operator*(const T lhs, const Brawler::Math::Quaternion& rhs)
{
	// a * b = b * a
	return rhs.MultiplyScalar(lhs);
}

template <typename T>
	requires std::is_arithmetic_v<T>
constexpr Brawler::Math::Quaternion operator/(const Brawler::Math::Quaternion& lhs, const T rhs)
{
	return lhs.DivideScalar(static_cast<float>(rhs));
}

template <typename T>
	requires std::is_arithmetic_v<T>
constexpr Brawler::Math::Quaternion operator/(const T lhs, const Brawler::Math::Quaternion& rhs)
{
	// a / b = a * (1 / b) = (1 / b) * a
	return rhs.GetReciprocal().MultiplyScalar(lhs);
}

constexpr Brawler::Math::Quaternion operator*(const Brawler::Math::Quaternion& lhs, const Brawler::Math::Quaternion& rhs)
{
	// I'm not sure as to how I should define this function. Mathematically, the only correct
	// way to define it would be lhs * rhs. However, most people use quaternions to "chain"
	// rotations together, and doing that requires multiplying rhs * lhs.
	//
	// Since quaternion multiplication is not commutative, these will inherently lead to different
	// results. Unlike DirectXMath, I am inclined to define the multiplication operation as
	// "true" quaternion multiplication. If the goal is to chain together rotations, then
	// Quaternion::ChainRotation() can be called, instead. Those who work from a more mathematical
	// perspective are more likely to understand (lhs * rhs) as being just that: lhs * rhs, and
	// not the chaining of two rotations.

	return lhs.MultiplyQuaternion(rhs);
}
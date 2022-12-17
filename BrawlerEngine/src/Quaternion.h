#pragma once
#include <utility>
#include <algorithm>
#include <source_location>
#include <DirectXMath/DirectXMath.h>
#include <string_view>
#include "Matrix.h"
#include "Vector.h"

import Util.Math;
import Util.General;

namespace Brawler
{
	namespace Math
	{
		namespace IMPL
		{
			class Quaternion
			{
			public:
				constexpr Quaternion() = default;
				constexpr explicit Quaternion(const Vector<float, 3>& normalizedRotationAxis, const float rotationInRadians);
				constexpr explicit Quaternion(const DirectX::XMFLOAT4& quaternionData);
				constexpr explicit Quaternion(const Matrix<3, 3>& rotationMatrix);

				constexpr Quaternion(const Quaternion& rhs) = default;
				constexpr Quaternion& operator=(const Quaternion& rhs) = default;

				constexpr Quaternion(Quaternion&& rhs) noexcept = default;
				constexpr Quaternion& operator=(Quaternion&& rhs) noexcept = default;

				constexpr Quaternion Normalize() const;
				constexpr float GetMagnitude() const;

				constexpr float GetMagnitudeSquared() const;

				constexpr float Dot(const Quaternion& rhs) const;

				constexpr float AngleBetween(const Quaternion& rhs) const;
				constexpr float AngleBetweenNormalized(const Quaternion& rhs) const;

				constexpr Quaternion AddQuaternion(const Quaternion& rhs) const;
				constexpr Quaternion SubtractQuaternion(const Quaternion& rhs) const;

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
				/// unit quaternions, this function first calculates the rotation created by first rotating 
				/// by this Quaternion instance and then rotating by nextRotationQuaternion. It then sets
				/// the value of this Quaternion instance to the calculated result. Finally, the function
				/// returns a reference to *this.
				/// 
				/// Mathematically, let p and q be two unit quaternions. Then, this function will
				/// return the result of qp, which represents a rotation by p followed by a rotation
				/// by q. Importantly, the quaternion qp will also be a unit quaternion; this allows
				/// for seamlessly chaining rotation quaternions.
				/// 
				/// As mentioned earlier, Quaternion::ChainRotation() *WILL* modify this Quaternion instance.
				/// To get the result of this operation without modifying it, there are two options available:
				/// 
				///   - Call p.GetChainedRotation(q). This will perform the same operation without
				///     modifying p and simply returns the result.
				/// 
				///   - Call q.MultiplyQuaternion(p) (or use q * p). This will perform the quaternion multiplication
				///     operation in a manner equivalent to Quaternion::GetChainedRotation().
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
				/// Then, assuming that both p and q are normalized, this function set the value of this
				/// Quaternion instance to the unit/normalized Quaternion qp. Finally, it returns a reference to
				/// *this for possible future chaining.
				/// </returns>
				constexpr Quaternion& ChainRotation(const Quaternion& nextRotationQuaternion);

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
				/// Unlike Quaternion::ChainRotation(), Quaternion::GetChainedRotation() does *NOT* modify
				/// the value of this Quaternion instance; instead, it only returns the result of qp.
				/// 
				/// Notably, calling p.GetChainedRotation(q) will return the same result as
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
				constexpr Quaternion GetChainedRotation(const Quaternion& nextRotationQuaternion) const;

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
				constexpr Matrix<3, 3> ConvertToRotationMatrix() const;

				/// <summary>
				/// Assuming that this Quaternion instance is a unit quaternion, this function returns the
				/// result of rotating a vector via the rotation specified by this Quaternion instance.
				/// 
				/// Mathematically, let q be a unit quaternion and v be a vector which is to be rotated in
				/// the manner specified by q. Then, this function returns the vector v' calculated as
				/// follows:
				/// 
				/// v' = (q)v(q^(-1)) = qvq*
				/// 
				/// (Note that for any unit quaternion q, q^(-1) = q* because the full equation for the
				/// inverse of a quaternion is q^(-1) = q*/(||q||^2).)
				/// 
				/// If the vector specified by rhs is a unit vector, then the returned vector will also be
				/// a unit vector. This is guaranteed because for any two quaternions p and q, ||pq|| =
				/// ||p|| * ||q||. (A vector can be considered a quaternion with a zero real part.)
				/// 
				/// Internally, this function will use a simplified equation to get the same result as qvq*.
				/// Thus, calling this function is preferred over performing the rotation manually.
				/// 
				/// *NOTE*: The function asserts in Debug builds if this Quaternion instance is unnormalized.
				/// </summary>
				/// <param name="rhs">
				/// - The vector which is to be rotated according to the rotation specified by this
				///   Quaternion instance.
				/// </param>
				/// <returns>
				/// The function returns the vector rhs rotated according to the rotation specified by
				/// this Quaternion instance. If rhs is a unit vector, then the returned vector will
				/// also be normalized.
				/// </returns>
				constexpr Vector<float, 3> RotateVector(const Vector<float, 3>& rhs) const;

				constexpr Quaternion Inverse() const;
				constexpr Quaternion Conjugate() const;

				constexpr bool IsNormalized() const;

				constexpr Quaternion GetReciprocal() const;

				constexpr Quaternion SLerp(const Quaternion& rhs, const float alpha) const;
				constexpr Quaternion SLerpNormalized(const Quaternion& rhs, const float alpha) const;

				constexpr float GetScalarComponent() const;
				constexpr Vector<float, 3> GetVectorComponent() const;

				DirectX::XMVECTOR GetDirectXMathVector() const;

				constexpr Quaternion& operator+=(const Quaternion& rhs);
				constexpr Quaternion& operator-=(const Quaternion& rhs);
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
}

namespace Brawler
{
	namespace Math
	{
		namespace IMPL
		{
			// NOTE: Moving the definitions of these functions into the Quaternion class definition like we do for
			// Vector and Matrix is causing the MSVC to throw internal compiler errors (ICEs) when importing
			// Brawler.Math.MathTypes.

			template <typename T>
				requires std::is_arithmetic_v<T>
			constexpr Brawler::Math::IMPL::Quaternion operator*(const Brawler::Math::IMPL::Quaternion& lhs, const T rhs);

			template <typename T>
				requires std::is_arithmetic_v<T>
			constexpr Brawler::Math::IMPL::Quaternion operator*(const T lhs, const Brawler::Math::IMPL::Quaternion& rhs);

			template <typename T>
				requires std::is_arithmetic_v<T>
			constexpr Brawler::Math::IMPL::Quaternion operator/(const Brawler::Math::IMPL::Quaternion& lhs, const T rhs);

			template <typename T>
				requires std::is_arithmetic_v<T>
			constexpr Brawler::Math::IMPL::Quaternion operator/(const T lhs, const Brawler::Math::IMPL::Quaternion& rhs);

			constexpr Brawler::Math::IMPL::Quaternion operator+(const Brawler::Math::IMPL::Quaternion& lhs, const Brawler::Math::IMPL::Quaternion& rhs);
			constexpr Brawler::Math::IMPL::Quaternion operator-(const Brawler::Math::IMPL::Quaternion& lhs, const Brawler::Math::IMPL::Quaternion& rhs);
			constexpr Brawler::Math::IMPL::Quaternion operator*(const Brawler::Math::IMPL::Quaternion& lhs, const Brawler::Math::IMPL::Quaternion& rhs);
		}
	}
}

// -------------------------------------------------------------------------------------------------------------------------------------------

namespace
{
	__forceinline constexpr void SafeAssert(const bool expression, const std::wstring_view assertMsg, const std::source_location srcLocation = std::source_location::current())
	{
		// Would you believe me if I told you that using the assert() macro in this file - and only this file - 
		// is causing the MSVC to spit out false errors?

#ifdef _DEBUG
		(void)(
			(!!(expression)) ||
			(_wassert(assertMsg.data(), L"Quaternion.h", srcLocation.line()), 0)
			);
#endif // _DEBUG
	}
}

namespace Brawler
{
	namespace Math
	{
		namespace IMPL
		{
			constexpr Quaternion::Quaternion(const Vector<float, 3>& normalizedRotationAxis, const float rotationInRadians) :
				mQuaternionVector()
			{
				SafeAssert(normalizedRotationAxis.IsNormalized(), L"ERROR: An unnormalized rotation axis was specified when constructing a Quaternion instance!");
				
				if (std::is_constant_evaluated())
				{
					const float halfAngle = (rotationInRadians / 2.0f);

					const float sineHalfAngle = Util::Math::GetSineAngle(halfAngle);
					const float cosineHalfAngle = Util::Math::GetCosineAngle(halfAngle);

					// We can store the quaternion as q = (sineHalfAngle * normalizedRotationAxis, cosineHalfAngle).
					const Vector<float, 3> scaledRotationNormal{ normalizedRotationAxis * sineHalfAngle };

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

			constexpr Quaternion::Quaternion(const Matrix<3, 3>& rotationMatrix) :
				mQuaternionVector()
			{
				if (std::is_constant_evaluated())
				{
					// Find the component of the quaternion with the largest absolute value.
					float quaternionXComparisonValue = (rotationMatrix.GetElement(0, 0) - rotationMatrix.GetElement(1, 1) - rotationMatrix.GetElement(2, 2) + 1.0f);
					
					if (quaternionXComparisonValue < 0.0f)
						quaternionXComparisonValue *= -1.0f;

					float quaternionYComparisonValue = (-(rotationMatrix.GetElement(0, 0)) + rotationMatrix.GetElement(1, 1) - rotationMatrix.GetElement(2, 2) + 1.0f);

					if (quaternionYComparisonValue < 0.0f)
						quaternionYComparisonValue *= -1.0f;

					float quaternionZComparisonValue = (-(rotationMatrix.GetElement(0, 0)) - rotationMatrix.GetElement(1, 1) + rotationMatrix.GetElement(2, 2) + 1.0f);

					if (quaternionZComparisonValue < 0.0f)
						quaternionZComparisonValue *= -1.0f;

					float quaternionWComparisonValue = (rotationMatrix.GetElement(0, 0) + rotationMatrix.GetElement(1, 1) + rotationMatrix.GetElement(2, 2) + 1.0f);

					if (quaternionWComparisonValue < 0.0f)
						quaternionWComparisonValue *= -1.0f;

					enum class QuaternionComponentID
					{
						COMPONENT_X,
						COMPONENT_Y,
						COMPONENT_Z,
						COMPONENT_W
					};

					QuaternionComponentID largestComponentID = QuaternionComponentID::COMPONENT_X;

					{
						float largestComparisonValue = quaternionXComparisonValue;

						if (quaternionYComparisonValue > largestComparisonValue)
						{
							largestComparisonValue = quaternionYComparisonValue;
							largestComponentID = QuaternionComponentID::COMPONENT_Y;
						}

						if (quaternionZComparisonValue > largestComparisonValue)
						{
							largestComparisonValue = quaternionZComparisonValue;
							largestComponentID = QuaternionComponentID::COMPONENT_Z;
						}

						if (quaternionWComparisonValue > largestComparisonValue)
						{
							largestComparisonValue = quaternionWComparisonValue;
							largestComponentID = QuaternionComponentID::COMPONENT_W;
						}
					}

					switch (largestComponentID)
					{
					case QuaternionComponentID::COMPONENT_X:
					{
						mQuaternionVector.x = (Util::Math::GetSquareRoot(quaternionXComparisonValue) * 0.5f);
						const float scalarValue = (0.25f / mQuaternionVector.x);

						mQuaternionVector.y = (rotationMatrix.GetElement(0, 1) + rotationMatrix.GetElement(1, 0)) * scalarValue;
						mQuaternionVector.z = (rotationMatrix.GetElement(2, 0) + rotationMatrix.GetElement(0, 2)) * scalarValue;
						mQuaternionVector.w = (rotationMatrix.GetElement(1, 2) - rotationMatrix.GetElement(2, 1)) * scalarValue;

						break;
					}

					case QuaternionComponentID::COMPONENT_Y:
					{
						mQuaternionVector.y = (Util::Math::GetSquareRoot(quaternionYComparisonValue) * 0.5f);
						const float scalarValue = (0.25f / mQuaternionVector.y);

						mQuaternionVector.x = (rotationMatrix.GetElement(0, 1) + rotationMatrix.GetElement(1, 0)) * scalarValue;
						mQuaternionVector.z = (rotationMatrix.GetElement(1, 2) + rotationMatrix.GetElement(2, 1)) * scalarValue;
						mQuaternionVector.w = (rotationMatrix.GetElement(2, 0) - rotationMatrix.GetElement(0, 2)) * scalarValue;

						break;
					}

					case QuaternionComponentID::COMPONENT_Z:
					{
						mQuaternionVector.z = (Util::Math::GetSquareRoot(quaternionZComparisonValue) * 0.5f);
						const float scalarValue = (0.25f / mQuaternionVector.z);

						mQuaternionVector.x = (rotationMatrix.GetElement(2, 0) + rotationMatrix.GetElement(0, 2)) * scalarValue;
						mQuaternionVector.y = (rotationMatrix.GetElement(1, 2) + rotationMatrix.GetElement(2, 1)) * scalarValue;
						mQuaternionVector.w = (rotationMatrix.GetElement(0, 1) - rotationMatrix.GetElement(1, 0)) * scalarValue;

						break;
					}

					default:
					{
						mQuaternionVector.w = (Util::Math::GetSquareRoot(quaternionWComparisonValue) * 0.5f);
						const float scalarValue = (0.25f / mQuaternionVector.w);

						mQuaternionVector.x = (rotationMatrix.GetElement(1, 2) - rotationMatrix.GetElement(2, 1)) * scalarValue;
						mQuaternionVector.y = (rotationMatrix.GetElement(2, 0) - rotationMatrix.GetElement(0, 2)) * scalarValue;
						mQuaternionVector.z = (rotationMatrix.GetElement(0, 1) - rotationMatrix.GetElement(1, 0)) * scalarValue;

						break;
					}
					}
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

			constexpr float Quaternion::Dot(const Quaternion& rhs) const
			{
				if (std::is_constant_evaluated())
				{
					return (mQuaternionVector.x * rhs.mQuaternionVector.x) + (mQuaternionVector.y * rhs.mQuaternionVector.y) + (mQuaternionVector.z * rhs.mQuaternionVector.z) +
						(mQuaternionVector.w * rhs.mQuaternionVector.w);
				}
				else
				{
					const DirectX::XMVECTOR loadedLHS{ DirectX::XMLoadFloat4(&mQuaternionVector) };
					const DirectX::XMVECTOR loadedRHS{ DirectX::XMLoadFloat4(&(rhs.mQuaternionVector)) };

					const DirectX::XMVECTOR dotProductVector{ DirectX::XMVector4Dot(loadedLHS, loadedRHS) };

					return DirectX::XMVectorGetX(dotProductVector);
				}
			}

			constexpr float Quaternion::AngleBetween(const Quaternion& rhs) const
			{
				if (std::is_constant_evaluated())
				{
					const Quaternion normalizedThis{ Normalize() };
					const Quaternion normalizedRHS{ rhs.Normalize() };

					return normalizedThis.AngleBetweenNormalized(normalizedRHS);
				}
				else
				{
					const DirectX::XMVECTOR loadedLHS{ DirectX::XMLoadFloat4(&mQuaternionVector) };
					const DirectX::XMVECTOR loadedRHS{ DirectX::XMLoadFloat4(&(rhs.mQuaternionVector)) };

					const DirectX::XMVECTOR angleVector{ DirectX::XMVector4AngleBetweenVectors(loadedLHS, loadedRHS) };

					return DirectX::XMVectorGetX(angleVector);
				}
			}

			constexpr float Quaternion::AngleBetweenNormalized(const Quaternion& rhs) const
			{
				SafeAssert(IsNormalized(), L"ERROR: Quaternion::AngleBetweenNormalized() was called on a Quaternion instance which was not normalized!");
				SafeAssert(rhs.IsNormalized(), L"ERROR: An unnormalized Quaternion instance was specified in a call to Quaternion::AngleBetweenNormalized()!");

				if (std::is_constant_evaluated())
				{
					// Represent the quaternions as four-dimensional vectors and use Vector::AngleBetweenNormalized().
					const Vector<float, 4> thisVector{ mQuaternionVector };
					const Vector<float, 4> rhsVector{ rhs.mQuaternionVector };

					return thisVector.AngleBetweenNormalized(rhsVector);
				}
				else
				{
					const DirectX::XMVECTOR loadedLHS{ DirectX::XMLoadFloat4(&mQuaternionVector) };
					const DirectX::XMVECTOR loadedRHS{ DirectX::XMLoadFloat4(&(rhs.mQuaternionVector)) };

					const DirectX::XMVECTOR angleVector{ DirectX::XMVector4AngleBetweenNormals(loadedLHS, loadedRHS) };

					return DirectX::XMVectorGetX(angleVector);
				}
			}

			constexpr Quaternion Quaternion::AddQuaternion(const Quaternion& rhs) const
			{
				if (std::is_constant_evaluated())
				{
					const DirectX::XMFLOAT4 addResult{
						(mQuaternionVector.x + rhs.mQuaternionVector.x),
						(mQuaternionVector.y + rhs.mQuaternionVector.y),
						(mQuaternionVector.z + rhs.mQuaternionVector.z),
						(mQuaternionVector.w + rhs.mQuaternionVector.w)
					};

					return Quaternion{ addResult };
				}
				else
				{
					const DirectX::XMVECTOR loadedLHS{ DirectX::XMLoadFloat4(&mQuaternionVector) };
					const DirectX::XMVECTOR loadedRHS{ DirectX::XMLoadFloat4(&(rhs.mQuaternionVector)) };

					const DirectX::XMVECTOR addResultVector{ DirectX::XMVectorAdd(loadedLHS, loadedRHS) };

					DirectX::XMFLOAT4 storedResult{};
					DirectX::XMStoreFloat4(&storedResult, addResultVector);

					return Quaternion{ storedResult };
				}
			}

			constexpr Quaternion Quaternion::SubtractQuaternion(const Quaternion& rhs) const
			{
				if (std::is_constant_evaluated())
				{
					const DirectX::XMFLOAT4 subtractResult{
						(mQuaternionVector.x - rhs.mQuaternionVector.x),
						(mQuaternionVector.y - rhs.mQuaternionVector.y),
						(mQuaternionVector.z - rhs.mQuaternionVector.z),
						(mQuaternionVector.w - rhs.mQuaternionVector.w)
					};

					return Quaternion{ subtractResult };
				}
				else
				{
					const DirectX::XMVECTOR loadedLHS{ DirectX::XMLoadFloat4(&mQuaternionVector) };
					const DirectX::XMVECTOR loadedRHS{ DirectX::XMLoadFloat4(&(rhs.mQuaternionVector)) };

					const DirectX::XMVECTOR subtractResultVector{ DirectX::XMVectorSubtract(loadedLHS, loadedRHS) };

					DirectX::XMFLOAT4 storedResult{};
					DirectX::XMStoreFloat4(&storedResult, subtractResultVector);

					return Quaternion{ storedResult };
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

					const Vector<float, 3> v1 = Vector<float, 3>{ DirectX::XMFLOAT3{ mQuaternionVector.x, mQuaternionVector.y, mQuaternionVector.z } };
					const float s1 = mQuaternionVector.w;

					const Vector<float, 3> v2 = Vector<float, 3>{ DirectX::XMFLOAT3{ rhs.mQuaternionVector.x, rhs.mQuaternionVector.y, rhs.mQuaternionVector.z } };
					const float s2 = rhs.mQuaternionVector.w;

					const Vector<float, 3> resultVectorComponent{ (s1 * v2) + (s2 * v1) + v1.Cross(v2) };
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

			constexpr Quaternion& Quaternion::ChainRotation(const Quaternion& nextRotationQuaternion)
			{
				// Consider two unit quaternions, p and q, both of which represent rotations. Then,
				// a quaternion r which represents a rotation specified by p followed by a rotation
				// specified by q is defined as r = qp.
				SafeAssert(IsNormalized(), L"ERROR: Quaternion::ChainRotation() was called on a Quaternion instance which was not normalized!");
				SafeAssert(nextRotationQuaternion.IsNormalized(), L"ERROR: An unnormalized Quaternion instance was specified in a call to Quaternion::ChainRotation()!");

				*this = nextRotationQuaternion.MultiplyQuaternion(*this);
				return *this;
			}

			constexpr Quaternion Quaternion::GetChainedRotation(const Quaternion& nextRotationQuaternion) const
			{
				SafeAssert(IsNormalized(), L"ERROR: Quaternion::GetChainedRotation() was called on a Quaternion instance which was not normalized!");
				SafeAssert(nextRotationQuaternion.IsNormalized(), L"ERROR: An unnormalized Quaternion instance was specified in a call to Quaternion::GetChainedRotation()!");
				
				return nextRotationQuaternion.MultiplyQuaternion(*this);
			}

			constexpr Matrix<3, 3> Quaternion::ConvertToRotationMatrix() const
			{
				// For some reason, setting the values of the returned Matrix<3, 3> instance via the constructor is causing
				// an MSVC internal compiler error. To work-around that, we use the default constructor of the class and
				// call Matrix::GetElement() to manually set each value.

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

					Matrix<3, 3> returnMatrix{};

					returnMatrix.GetElement(0, 0) = (1.0f - (2.0f * quaternionYSquared) - (2.0f * quaternionZSquared));
					returnMatrix.GetElement(0, 1) = (twoTimesXY + twoTimesZW);
					returnMatrix.GetElement(0, 2) = (twoTimesXZ - twoTimesYW);

					returnMatrix.GetElement(1, 0) = (twoTimesXY - twoTimesZW);
					returnMatrix.GetElement(1, 1) = (1.0f - (2.0f * quaternionXSquared) - (2.0f * quaternionZSquared));
					returnMatrix.GetElement(1, 2) = (twoTimesYZ + twoTimesXW);

					returnMatrix.GetElement(2, 0) = (twoTimesXZ + twoTimesYW);
					returnMatrix.GetElement(2, 1) = (twoTimesYZ - twoTimesXW);
					returnMatrix.GetElement(2, 2) = (1.0f - (2.0f * quaternionXSquared) - (2.0f * quaternionYSquared));

					return returnMatrix;
				}
				else
				{
					const DirectX::XMVECTOR loadedThis{ DirectX::XMLoadFloat4(&mQuaternionVector) };

					const DirectX::XMMATRIX loadedRotationMatrix{ DirectX::XMMatrixRotationQuaternion(loadedThis) };

					DirectX::XMFLOAT3X3 storedResult{};
					DirectX::XMStoreFloat3x3(&storedResult, loadedRotationMatrix);

					Matrix<3, 3> returnMatrix{};

					returnMatrix.GetElement(0, 0) = storedResult(0, 0);
					returnMatrix.GetElement(0, 1) = storedResult(0, 1);
					returnMatrix.GetElement(0, 2) = storedResult(0, 2);

					returnMatrix.GetElement(1, 0) = storedResult(1, 0);
					returnMatrix.GetElement(1, 1) = storedResult(1, 1);
					returnMatrix.GetElement(1, 2) = storedResult(1, 2);

					returnMatrix.GetElement(2, 0) = storedResult(2, 0);
					returnMatrix.GetElement(2, 1) = storedResult(2, 1);
					returnMatrix.GetElement(2, 2) = storedResult(2, 2);

					return returnMatrix;
				}
			}

			constexpr Vector<float, 3> Quaternion::RotateVector(const Vector<float, 3>& rhs) const
			{
				SafeAssert(IsNormalized(), L"ERROR: Quaternion::RotateVector() was called on a Quaternion instance which was not normalized!");

				// Mathematically, rotating a vector v according to a unit quaternion q can be achieved as follows:
				//
				// v' = (q)v(q^(-1)) = qvq* (For unit quaternions, q^(-1) = q*.)
				//
				// This is the correct and generalized equation, but it would involve a lot of operations.
				// As it turns out, there is a faster way to do this for unit quaternions, as explained
				// at https://fgiesen.wordpress.com/2019/02/09/rotating-a-single-vector-using-a-quaternion/:
				//
				// t = (2 * q.xyz) x v
				// v' = v + (q.w * t) + (q.xyz x t)
				
				const Vector<float, 3> imaginaryPart{ GetVectorComponent() };
				const Vector<float, 3> twoTimesImaginaryPart{ imaginaryPart * 2.0f };

				const Vector<float, 3> t{ twoTimesImaginaryPart.Cross(rhs) };

				return (rhs + (GetScalarComponent() * t) + imaginaryPart.Cross(t));
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

			constexpr Quaternion Quaternion::SLerp(const Quaternion& rhs, const float alpha) const
			{
				const Quaternion normalizedThis{ Normalize() };
				const Quaternion normalizedRHS{ rhs.Normalize() };

				return normalizedThis.SLerpNormalized(normalizedRHS, alpha);
			}

			constexpr Quaternion Quaternion::SLerpNormalized(const Quaternion& rhs, const float alpha) const
			{
				SafeAssert(IsNormalized(), L"ERROR: Quaternion::SLerpNormalized() was called on a Quaternion instance which was not normalized!");
				SafeAssert(rhs.IsNormalized(), L"ERROR: An unnormalized Quaternion instance was specified in a call to Quaternion::SLerpNormalized()!");
				
				// Ensure that alpha is in the range [0.0f, 1.0f].
				const float clampedAlpha = std::clamp(alpha, 0.0f, 1.0f);

				if (std::is_constant_evaluated())
				{
					// Choose either rhs or -rhs depending on which one produces the shortest arc along
					// the 4D unit sphere. This can be checked by comparing ||this - rhs||^2 to ||this + rhs||^2.
					//
					// If ||this + rhs||^2 < ||this - rhs||^2, then we choose -rhs; otherwise, we choose rhs.
					const float thisPlusRhsMagnitudeSquared = Quaternion{ *this + rhs }.GetMagnitudeSquared();
					const float thisMinusRhsMagnitudeSquared = Quaternion{ *this - rhs }.GetMagnitudeSquared();

					const Quaternion comparisonQuaternion{ thisPlusRhsMagnitudeSquared < thisMinusRhsMagnitudeSquared ? (rhs * -1.0f) : rhs };

					const float angleBetween = AngleBetweenNormalized(comparisonQuaternion);
					const float sineAngleBetween = Util::Math::GetSineAngle(angleBetween);
					const float sineAngleTimesAlpha = Util::Math::GetSineAngle(angleBetween * clampedAlpha);
					const float sineAngleTimesOneMinusAlpha = Util::Math::GetSineAngle(angleBetween * (1.0f - clampedAlpha));

					// Both this and rhs/-rhs are unit quaternions on the 4D unit sphere. Spherical interpolation works
					// by finding a unit quaternion which lies along the path between these two quaternions on the
					// unit sphere. Since the interpolated quaternion also lies on the 4D unit sphere, it must
					// also be a unit quaternion.

					return (((sineAngleTimesOneMinusAlpha * *this) + (sineAngleTimesAlpha * comparisonQuaternion)) / sineAngleBetween);
				}
				else
				{
					const DirectX::XMVECTOR loadedLHS{ DirectX::XMLoadFloat4(&mQuaternionVector) };
					const DirectX::XMVECTOR loadedRHS{ DirectX::XMLoadFloat4(&(rhs.mQuaternionVector)) };

					const DirectX::XMVECTOR slerpQuaternion{ DirectX::XMQuaternionSlerp(loadedLHS, loadedRHS, clampedAlpha) };

					DirectX::XMFLOAT4 storedResult{};
					DirectX::XMStoreFloat4(&storedResult, slerpQuaternion);

					return Quaternion{ storedResult };
				}
			}

			constexpr float Quaternion::GetScalarComponent() const
			{
				return mQuaternionVector.w;
			}

			constexpr Vector<float, 3> Quaternion::GetVectorComponent() const
			{
				return Vector<float, 3>{ DirectX::XMFLOAT3{
					mQuaternionVector.x,
					mQuaternionVector.y,
					mQuaternionVector.z
				} };
			}

			DirectX::XMVECTOR Quaternion::GetDirectXMathVector() const
			{
				return DirectX::XMLoadFloat4(&mQuaternionVector);
			}

			constexpr Quaternion& Quaternion::operator+=(const Quaternion& rhs)
			{
				*this = AddQuaternion(rhs);
				return *this;
			}

			constexpr Quaternion& Quaternion::operator-=(const Quaternion& rhs)
			{
				*this = SubtractQuaternion(rhs);
				return *this;
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
}

namespace Brawler
{
	namespace Math
	{
		namespace IMPL
		{
			template <typename T>
				requires std::is_arithmetic_v<T>
			constexpr Brawler::Math::IMPL::Quaternion operator*(const Brawler::Math::IMPL::Quaternion& lhs, const T rhs)
			{
				return lhs.MultiplyScalar(static_cast<float>(rhs));
			}

			template <typename T>
				requires std::is_arithmetic_v<T>
			constexpr Brawler::Math::IMPL::Quaternion operator*(const T lhs, const Brawler::Math::IMPL::Quaternion& rhs)
			{
				// a * b = b * a
				return rhs.MultiplyScalar(lhs);
			}

			template <typename T>
				requires std::is_arithmetic_v<T>
			constexpr Brawler::Math::IMPL::Quaternion operator/(const Brawler::Math::IMPL::Quaternion& lhs, const T rhs)
			{
				return lhs.DivideScalar(static_cast<float>(rhs));
			}

			template <typename T>
				requires std::is_arithmetic_v<T>
			constexpr Brawler::Math::IMPL::Quaternion operator/(const T lhs, const Brawler::Math::IMPL::Quaternion& rhs)
			{
				// a / b = a * (1 / b) = (1 / b) * a
				return rhs.GetReciprocal().MultiplyScalar(lhs);
			}

			constexpr Brawler::Math::IMPL::Quaternion operator+(const Brawler::Math::IMPL::Quaternion& lhs, const Brawler::Math::IMPL::Quaternion& rhs)
			{
				return lhs.AddQuaternion(rhs);
			}

			constexpr Brawler::Math::IMPL::Quaternion operator-(const Brawler::Math::IMPL::Quaternion& lhs, const Brawler::Math::IMPL::Quaternion& rhs)
			{
				return lhs.SubtractQuaternion(rhs);
			}

			constexpr Brawler::Math::IMPL::Quaternion operator*(const Brawler::Math::IMPL::Quaternion& lhs, const Brawler::Math::IMPL::Quaternion& rhs)
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
		}
	}
}
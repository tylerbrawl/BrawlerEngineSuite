module;
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <ranges>
#include <DirectXMath/DirectXMath.h>

export module Brawler.Math.MathTypes:Vector;
import :Matrix;
import Util.Math;

namespace Brawler
{
	namespace Math
	{
		template <typename ElementType, std::size_t NumElements>
		struct VectorInfo
		{
			static_assert(sizeof(NumElements) != sizeof(NumElements));
		};

		template <typename RetType, typename... Args>
		using VectorCallFunctionPtr = RetType(XM_CALLCONV*)(Args...);

		template <
			typename StorageType_, 
			auto LoadFunctionPtr,
			auto StoreFunctionPtr
		>
		struct VectorInfoInstantiation
		{
			using StorageType = StorageType_;
			using MathType = DirectX::XMVECTOR;

			static constexpr auto LOAD_FUNCTION{ LoadFunctionPtr };
			static constexpr auto STORE_FUNCTION{ StoreFunctionPtr };
		};

		template <>
		struct VectorInfo<std::int32_t, 2> : public VectorInfoInstantiation<DirectX::XMINT2, DirectX::XMLoadInt2, DirectX::XMStoreInt2>
		{};

		template <>
		struct VectorInfo<std::uint32_t, 2> : public VectorInfoInstantiation<DirectX::XMUINT2, DirectX::XMLoadUInt2, DirectX::XMStoreUInt2>
		{};

		template <>
		struct VectorInfo<float, 2> : public VectorInfoInstantiation<DirectX::XMFLOAT2, DirectX::XMLoadFloat2, DirectX::XMStoreFloat2>
		{};

		template <>
		struct VectorInfo<std::int32_t, 3> : public VectorInfoInstantiation<DirectX::XMINT3, DirectX::XMLoadInt3, DirectX::XMStoreInt3>
		{};

		template <>
		struct VectorInfo<std::uint32_t, 3> : public VectorInfoInstantiation<DirectX::XMUINT3, DirectX::XMLoadUInt3, DirectX::XMStoreUInt3>
		{};

		template <>
		struct VectorInfo<float, 3> : public VectorInfoInstantiation<DirectX::XMFLOAT3, DirectX::XMLoadFloat3, DirectX::XMStoreFloat3>
		{};

		template <>
		struct VectorInfo<std::int32_t, 4> : public VectorInfoInstantiation<DirectX::XMINT4, DirectX::XMLoadInt4, DirectX::XMStoreInt4>
		{};

		template <>
		struct VectorInfo<std::uint32_t, 4> : public VectorInfoInstantiation<DirectX::XMUINT4, DirectX::XMLoadUInt4, DirectX::XMStoreUInt4>
		{};

		template <>
		struct VectorInfo<float, 4> : public VectorInfoInstantiation<DirectX::XMFLOAT4, DirectX::XMLoadFloat4, DirectX::XMStoreFloat4>
		{};
	}
}

namespace Brawler
{
	namespace Math
	{
		template <typename ElementType, std::size_t NumElements>
		class Vector
		{
		private:
			using StorageType = typename VectorInfo<ElementType, NumElements>::StorageType;
			using MathType = typename VectorInfo<ElementType, NumElements>::MathType;

			static constexpr auto LOAD_FUNCTION = VectorInfo<ElementType, NumElements>::LOAD_FUNCTION;
			static constexpr auto STORE_FUNCTION = VectorInfo<ElementType, NumElements>::STORE_FUNCTION;

		public:
			constexpr Vector() = default;
			constexpr explicit Vector(const StorageType& vectorData);
			constexpr explicit Vector(const ElementType replicatedScalar);

			constexpr Vector(const Vector& rhs) = default;
			constexpr Vector& operator=(const Vector& rhs) = default;

			constexpr Vector(Vector&& rhs) noexcept = default;
			constexpr Vector& operator=(Vector&& rhs) noexcept = default;

			constexpr Vector AddVector(const Vector& rhs) const;
			constexpr Vector SubtractVector(const Vector& rhs) const;
			constexpr Vector PerComponentMultiplyVector(const Vector& rhs) const;
			constexpr Vector PerComponentDivideVector(const Vector& rhs) const;

			constexpr Vector AddScalar(const ElementType rhs) const;
			constexpr Vector SubtractScalar(const ElementType rhs) const;
			constexpr Vector MultiplyScalar(const ElementType rhs) const;
			constexpr Vector DivideScalar(const ElementType rhs) const;

			template <std::size_t NumColumns>
			constexpr Vector<float, NumColumns> TransformByMatrix(const Matrix<NumElements, NumColumns>& rhs) const;

			constexpr ElementType Dot(const Vector& rhs) const;
			constexpr Vector Cross(const Vector& rhs) const requires (NumElements == 3);

			constexpr Vector<float, NumElements> Normalize() const;
			constexpr float GetLength() const;

			constexpr float AngleBetween(const Vector& rhs) const;
			constexpr float AngleBetweenNormalized(const Vector& rhs) const;

			constexpr Vector<float, NumElements> GetReciprocal() const;

			constexpr bool IsNormalized() const;

			constexpr ElementType GetX() const;
			constexpr ElementType GetY() const;
			constexpr ElementType GetZ() const requires (NumElements >= 3);
			constexpr ElementType GetW() const requires (NumElements >= 4);

			MathType GetDirectXMathVector() const;

			constexpr Vector& operator+=(const Vector& rhs);
			constexpr Vector& operator-=(const Vector& rhs);
			constexpr Vector& operator*=(const Vector& rhs);
			constexpr Vector& operator/=(const Vector& rhs);

			constexpr Vector& operator+=(const ElementType rhs);
			constexpr Vector& operator-=(const ElementType rhs);
			constexpr Vector& operator*=(const ElementType rhs);
			constexpr Vector& operator/=(const ElementType rhs);

			constexpr Vector& operator*=(const Matrix<NumElements, NumElements>& rhs);

		private:
			StorageType mStoredVector;
		};
	}
}

export
{
	template <typename ElementType, std::size_t NumElements>
	constexpr Brawler::Math::Vector<ElementType, NumElements> operator+(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const Brawler::Math::Vector<ElementType, NumElements>& rhs);

	template <typename ElementType, std::size_t NumElements>
	constexpr Brawler::Math::Vector<ElementType, NumElements> operator-(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const Brawler::Math::Vector<ElementType, NumElements>& rhs);

	template <typename ElementType, std::size_t NumElements>
	constexpr Brawler::Math::Vector<ElementType, NumElements> operator*(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const Brawler::Math::Vector<ElementType, NumElements>& rhs);

	template <typename ElementType, std::size_t NumElements>
	constexpr Brawler::Math::Vector<ElementType, NumElements> operator/(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const Brawler::Math::Vector<ElementType, NumElements>& rhs);

	template <typename ElementType, std::size_t NumElements>
	constexpr Brawler::Math::Vector<ElementType, NumElements> operator+(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const ElementType rhs);

	template <typename ElementType, std::size_t NumElements>
	constexpr Brawler::Math::Vector<ElementType, NumElements> operator-(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const ElementType rhs);

	template <typename ElementType, std::size_t NumElements>
	constexpr Brawler::Math::Vector<ElementType, NumElements> operator*(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const ElementType rhs);

	template <typename ElementType, std::size_t NumElements>
	constexpr Brawler::Math::Vector<ElementType, NumElements> operator/(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const ElementType rhs);

	template <typename ElementType, std::size_t NumElements, std::size_t MatrixNumColumns>
	constexpr Brawler::Math::Vector<ElementType, MatrixNumColumns> operator*(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const Brawler::Math::Matrix<NumElements, MatrixNumColumns>& rhs);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace Math
	{
		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements>::Vector(const StorageType& vectorData) :
			mStoredVector(vectorData)
		{}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements>::Vector(const ElementType replicatedScalar) :
			mStoredVector()
		{
			mStoredVector.x = replicatedScalar;
			mStoredVector.y = replicatedScalar;

			if constexpr (NumElements >= 3)
				mStoredVector.z = replicatedScalar;

			if constexpr (NumElements >= 4)
				mStoredVector.w = replicatedScalar;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements> Vector<ElementType, NumElements>::AddVector(const Vector& rhs) const
		{
			if (std::is_constant_evaluated())
			{
				StorageType resultVector{};
				resultVector.x = (mStoredVector.x + rhs.mStoredVector.x);
				resultVector.y = (mStoredVector.y + rhs.mStoredVector.y);

				if constexpr (NumElements >= 3)
					resultVector.z = (mStoredVector.z + rhs.mStoredVector.z);

				if constexpr (NumElements >= 4)
					resultVector.w = (mStoredVector.w + rhs.mStoredVector.w);

				return Vector{ resultVector };
			}
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				const MathType loadedRHS{ LOAD_FUNCTION(&(rhs.mStoredVector)) };

				const MathType addResult{ DirectX::XMVectorAdd(loadedLHS, loadedRHS) };

				StorageType storedResult{};
				STORE_FUNCTION(&storedResult, addResult);

				return Vector{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements> Vector<ElementType, NumElements>::SubtractVector(const Vector& rhs) const
		{
			if (std::is_constant_evaluated())
			{
				StorageType resultVector{};
				resultVector.x = (mStoredVector.x - rhs.mStoredVector.x);
				resultVector.y = (mStoredVector.y - rhs.mStoredVector.y);

				if constexpr (NumElements >= 3)
					resultVector.z = (mStoredVector.z - rhs.mStoredVector.z);

				if constexpr (NumElements >= 4)
					resultVector.w = (mStoredVector.w - rhs.mStoredVector.w);

				return Vector{ resultVector };
			}
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				const MathType loadedRHS{ LOAD_FUNCTION(&(rhs.mStoredVector)) };

				const MathType subtractResult{ DirectX::XMVectorSubtract(loadedLHS, loadedRHS) };

				StorageType storedResult{};
				STORE_FUNCTION(&storedResult, subtractResult);

				return Vector{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements> Vector<ElementType, NumElements>::PerComponentMultiplyVector(const Vector& rhs) const
		{
			if (std::is_constant_evaluated())
			{
				StorageType resultVector{};
				resultVector.x = (mStoredVector.x * rhs.mStoredVector.x);
				resultVector.y = (mStoredVector.y * rhs.mStoredVector.y);

				if constexpr (NumElements >= 3)
					resultVector.z = (mStoredVector.z * rhs.mStoredVector.z);

				if constexpr (NumElements >= 4)
					resultVector.w = (mStoredVector.w * rhs.mStoredVector.w);

				return Vector{ resultVector };
			}
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				const MathType loadedRHS{ LOAD_FUNCTION(&(rhs.mStoredVector)) };

				const MathType multiplyResult{ DirectX::XMVectorMultiply(loadedLHS, loadedRHS) };

				StorageType storedResult{};
				STORE_FUNCTION(&storedResult, multiplyResult);

				return Vector{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements> Vector<ElementType, NumElements>::PerComponentDivideVector(const Vector& rhs) const
		{
			if (std::is_constant_evaluated())
			{
				StorageType resultVector{};
				resultVector.x = (mStoredVector.x / rhs.mStoredVector.x);
				resultVector.y = (mStoredVector.y / rhs.mStoredVector.y);

				if constexpr (NumElements >= 3)
					resultVector.z = (mStoredVector.z / rhs.mStoredVector.z);

				if constexpr (NumElements >= 4)
					resultVector.w = (mStoredVector.w / rhs.mStoredVector.w);

				return Vector{ resultVector };
			}
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				const MathType loadedRHS{ LOAD_FUNCTION(&(rhs.mStoredVector)) };

				const MathType divideResult{ DirectX::XMVectorDivide(loadedLHS, loadedRHS) };

				StorageType storedResult{};
				STORE_FUNCTION(&storedResult, divideResult);

				return Vector{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements> Vector<ElementType, NumElements>::AddScalar(const ElementType rhs) const
		{
			if (std::is_constant_evaluated())
				return AddVector(Vector{ rhs });
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				MathType addResult{};

				if constexpr (std::is_same_v<ElementType, float>)
					addResult = DirectX::XMVectorAdd(loadedLHS, DirectX::XMVectorReplicate(rhs));
				else
					addResult = DirectX::XMVectorAdd(loadedLHS, DirectX::XMVectorReplicateInt(static_cast<std::uint32_t>(rhs)));

				StorageType storedResult{};
				STORE_FUNCTION(&storedResult, addResult);

				return Vector{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements> Vector<ElementType, NumElements>::SubtractScalar(const ElementType rhs) const
		{
			if (std::is_constant_evaluated())
				return SubtractVector(Vector{ rhs });
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				MathType subtractResult{};

				if constexpr (std::is_same_v<ElementType, float>)
					subtractResult = DirectX::XMVectorSubtract(loadedLHS, DirectX::XMVectorReplicate(rhs));
				else
					subtractResult = DirectX::XMVectorSubtract(loadedLHS, DirectX::XMVectorReplicateInt(static_cast<std::uint32_t>(rhs)));

				StorageType storedResult{};
				STORE_FUNCTION(&storedResult, subtractResult);

				return Vector{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements> Vector<ElementType, NumElements>::MultiplyScalar(const ElementType rhs) const
		{
			if (std::is_constant_evaluated())
				return PerComponentMultiplyVector(Vector{ rhs });
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				MathType multiplyResult{};

				if constexpr (std::is_same_v<ElementType, float>)
					multiplyResult = DirectX::XMVectorMultiply(loadedLHS, DirectX::XMVectorReplicate(rhs));
				else
					multiplyResult = DirectX::XMVectorMultiply(loadedLHS, DirectX::XMVectorReplicateInt(static_cast<std::uint32_t>(rhs)));

				StorageType storedResult{};
				STORE_FUNCTION(&storedResult, multiplyResult);

				return Vector{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements> Vector<ElementType, NumElements>::DivideScalar(const ElementType rhs) const
		{
			if (std::is_constant_evaluated())
				return PerComponentDivideVector(Vector{ rhs });
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				MathType divideResult{};

				if constexpr (std::is_same_v<ElementType, float>)
					divideResult = DirectX::XMVectorDivide(loadedLHS, DirectX::XMVectorReplicate(rhs));
				else
					divideResult = DirectX::XMVectorDivide(loadedLHS, DirectX::XMVectorReplicateInt(static_cast<std::uint32_t>(rhs)));

				StorageType storedResult{};
				STORE_FUNCTION(&storedResult, divideResult);

				return Vector{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		template <std::size_t NumColumns>
		constexpr Vector<float, NumColumns> Vector<ElementType, NumElements>::TransformByMatrix(const Matrix<NumElements, NumColumns>& rhs) const
		{
			if (std::is_constant_evaluated())
			{
				typename VectorInfo<float, NumColumns>::StorageType storedResult{};
				using MatrixColumnStorageType = typename VectorInfo<float, NumElements>::StorageType;

				constexpr auto GET_MATRIX_COLUMN_VECTOR_LAMBDA = [] (const Matrix<NumElements, NumColumns>& rhs, const std::size_t columnIndex)
				{
					if constexpr (NumElements == 2)
					{
						return Vector<float, NumElements>{DirectX::XMFLOAT2{
							rhs.GetElement(0, columnIndex),
							rhs.GetElement(1, columnIndex)
						} };
					}
					else if constexpr (NumElements == 3)
					{
						return Vector<float, NumElements>{DirectX::XMFLOAT3{
							rhs.GetElement(0, columnIndex),
							rhs.GetElement(1, columnIndex),
							rhs.GetElement(2, columnIndex)
						} };
					}
					else if constexpr (NumElements == 4)
					{
						return Vector<float, NumElements>{DirectX::XMFLOAT4{
							rhs.GetElement(0, columnIndex),
							rhs.GetElement(1, columnIndex),
							rhs.GetElement(2, columnIndex),
							rhs.GetElement(3, columnIndex)
						} };
					}
					else
						return Vector<float, NumElements>{};
				};

				// Create a casted version of *this so that Vector<std::int32_t, ...>::Dot() or Vector<std::uint32_t, ...>::Dot()
				// does not result in lost information.
				Vector<float, NumElements> castedThis{};

				if constexpr (NumElements == 2)
				{
					castedThis = Vector<float, NumElements>{ DirectX::XMFLOAT2{
						static_cast<float>(mStoredVector.x),
						static_cast<float>(mStoredVector.y)
					} };
				}
				else if constexpr (NumElements == 3)
				{
					castedThis = Vector<float, NumElements>{ DirectX::XMFLOAT3{
						static_cast<float>(mStoredVector.x),
						static_cast<float>(mStoredVector.y),
						static_cast<float>(mStoredVector.z)
					} };
				}
				else if constexpr (NumElements == 4)
				{
					castedThis = Vector<float, NumElements>{ DirectX::XMFLOAT4{
						static_cast<float>(mStoredVector.x),
						static_cast<float>(mStoredVector.y),
						static_cast<float>(mStoredVector.z),
						static_cast<float>(mStoredVector.w)
					} };
				}

				if constexpr (NumColumns >= 1)
				{
					const Vector<float, NumElements> matrixColumnOneVector{ GET_MATRIX_COLUMN_VECTOR_LAMBDA(rhs, 0) };
					storedResult.x = castedThis.Dot(matrixColumnOneVector);
				}

				if constexpr (NumColumns >= 2)
				{
					const Vector<float, NumElements> matrixColumnTwoVector{ GET_MATRIX_COLUMN_VECTOR_LAMBDA(rhs, 1) };
					storedResult.y = castedThis.Dot(matrixColumnTwoVector);
				}

				if constexpr (NumElements >= 3 && NumColumns >= 3)
				{
					const Vector<float, NumElements> matrixColumnThreeVector{ GET_MATRIX_COLUMN_VECTOR_LAMBDA(rhs, 2) };
					storedResult.z = castedThis.Dot(matrixColumnThreeVector);
				}

				if constexpr (NumElements >= 4 && NumColumns >= 4)
				{
					const Vector<float, NumElements> matrixColumnFourVector{ GET_MATRIX_COLUMN_VECTOR_LAMBDA(rhs, 3) };
					storedResult.w = castedThis.Dot(matrixColumnFourVector);
				}

				return Vector<float, NumColumns>{ storedResult };
			}
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				const DirectX::XMMATRIX loadedRHS{ rhs.GetDirectXMathMatrix() };

				MathType transformedVector{};

				if constexpr (NumElements == 2)
					transformedVector = DirectX::XMVector2Transform(loadedLHS, loadedRHS);
				else if constexpr (NumElements == 3)
					transformedVector = DirectX::XMVector3Transform(loadedLHS, loadedRHS);
				else if constexpr (NumElements == 4)
					transformedVector = DirectX::XMVector4Transform(loadedLHS, loadedRHS);

				typename VectorInfo<float, NumColumns>::StorageType storedResult{};
				VectorInfo<float, NumColumns>::STORE_FUNCTION(&storedResult, transformedVector);

				return Vector<float, NumColumns>{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr ElementType Vector<ElementType, NumElements>::Dot(const Vector& rhs) const
		{
			if (std::is_constant_evaluated())
			{
				ElementType currDotProductSum = 0;
				currDotProductSum += (mStoredVector.x * rhs.mStoredVector.x);
				currDotProductSum += (mStoredVector.y * rhs.mStoredVector.y);

				if constexpr (NumElements >= 3)
					currDotProductSum += (mStoredVector.z * rhs.mStoredVector.z);

				if constexpr (NumElements >= 4)
					currDotProductSum += (mStoredVector.w * rhs.mStoredVector.w);

				return currDotProductSum;
			}
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				const MathType loadedRHS{ LOAD_FUNCTION(&(rhs.mStoredVector)) };

				MathType dotProductVector{};

				if constexpr (NumElements == 2)
					dotProductVector = DirectX::XMVector2Dot(loadedLHS, loadedRHS);
				else if constexpr (NumElements == 3)
					dotProductVector = DirectX::XMVector3Dot(loadedLHS, loadedRHS);
				else if constexpr (NumElements == 4)
					dotProductVector = DirectX::XMVector4Dot(loadedLHS, loadedRHS);

				if constexpr (std::is_same_v<ElementType, float>)
					return DirectX::XMVectorGetX(dotProductVector);
				else
					return static_cast<ElementType>(DirectX::XMVectorGetIntX(dotProductVector));
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements> Vector<ElementType, NumElements>::Cross(const Vector& rhs) const requires (NumElements == 3)
		{
			if (std::is_constant_evaluated())
			{
				// The following equation can easily be derived by manually taking the cross product
				// of two three-dimensional vectors with generic values:
				StorageType storedResult{};
				storedResult.x = ((mStoredVector.y * rhs.mStoredVector.z) - (mStoredVector.z * rhs.mStoredVector.y));
				storedResult.y = ((mStoredVector.z * rhs.mStoredVector.x) - (mStoredVector.x * rhs.mStoredVector.z));
				storedResult.z = ((mStoredVector.x * rhs.mStoredVector.y) - (mStoredVector.y * rhs.mStoredVector.x));

				// The DirectXMath function DirectX::XMVector3Cross() function sets the resulting W-component
				// to zero, so we will leave it like that in storedResult.

				return Vector{ storedResult };
			}
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				const MathType loadedRHS{ LOAD_FUNCTION(&(rhs.mStoredVector)) };

				// DirectXMath does provide additional "cross product" functions for 2- and 4-dimensional
				// vectors, but these have no real geometrical meaning.
				MathType crossProductVector{};
				crossProductVector = DirectX::XMVector3Cross(loadedLHS, loadedRHS);

				StorageType storedResult{};
				STORE_FUNCTION(&storedResult, crossProductVector);

				return Vector{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<float, NumElements> Vector<ElementType, NumElements>::Normalize() const
		{
			if (std::is_constant_evaluated())
			{
				const float vectorMagnitude = GetLength();

				typename VectorInfo<float, NumElements>::StorageType storedResult{};
				storedResult.x = (mStoredVector.x / vectorMagnitude);
				storedResult.y = (mStoredVector.y / vectorMagnitude);

				if constexpr (NumElements >= 3)
					storedResult.z = (mStoredVector.z / vectorMagnitude);

				if constexpr (NumElements >= 4)
					storedResult.w = (mStoredVector.w / vectorMagnitude);

				return Vector<float, NumElements>{ storedResult };
			}
			else
			{
				const MathType loadedThis{ LOAD_FUNCTION(&mStoredVector) };
				MathType normalizedVector{};

				if constexpr (NumElements == 2)
					normalizedVector = DirectX::XMVector2Normalize(loadedThis);
				else if constexpr (NumElements == 3)
					normalizedVector = DirectX::XMVector3Normalize(loadedThis);
				else if constexpr (NumElements == 4)
					normalizedVector = DirectX::XMVector4Normalize(loadedThis);

				typename VectorInfo<float, NumElements>::StorageType storedResult{};
				VectorInfo<float, NumElements>::STORE_FUNCTION(&storedResult, normalizedVector);

				return Vector<float, NumElements>{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr float Vector<ElementType, NumElements>::GetLength() const
		{
			if (std::is_constant_evaluated())
			{
				ElementType elementsSum = 0;
				elementsSum += (mStoredVector.x * mStoredVector.x);
				elementsSum += (mStoredVector.y * mStoredVector.y);

				if constexpr (NumElements >= 3)
					elementsSum += (mStoredVector.z * mStoredVector.z);

				if constexpr (NumElements >= 4)
					elementsSum += (mStoredVector.w * mStoredVector.w);

				if (elementsSum == 0) [[unlikely]]
					return 0;

				return Util::Math::GetSquareRoot(elementsSum);
			}
			else
			{
				const MathType loadedThis{ LOAD_FUNCTION(&mStoredVector) };
				MathType lengthVector{};

				if constexpr (NumElements == 2)
					lengthVector = DirectX::XMVector2Length(loadedThis);
				else if constexpr (NumElements == 3)
					lengthVector = DirectX::XMVector3Length(loadedThis);
				else if constexpr (NumElements == 4)
					lengthVector = DirectX::XMVector4Length(loadedThis);

				return DirectX::XMVectorGetX(lengthVector);
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr float Vector<ElementType, NumElements>::AngleBetween(const Vector& rhs) const
		{
			if (std::is_constant_evaluated())
			{
				const Vector<float, NumElements> normalizedLHS{ Normalize() };
				const Vector<float, NumElements> normalizedRHS{ rhs.Normalize() };

				return normalizedLHS.AngleBetweenNormalized(normalizedRHS);
			}
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				const MathType loadedRHS{ LOAD_FUNCTION(&(rhs.mStoredVector)) };

				MathType angleVector{};

				if constexpr (NumElements == 2)
					angleVector = DirectX::XMVector2AngleBetweenVectors(loadedLHS, loadedRHS);
				else if constexpr (NumElements == 3)
					angleVector = DirectX::XMVector3AngleBetweenVectors(loadedLHS, loadedRHS);
				else if constexpr (NumElements == 4)
					angleVector = DirectX::XMVector4AngleBetweenVectors(loadedLHS, loadedRHS);

				return DirectX::XMVectorGetX(angleVector);
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr float Vector<ElementType, NumElements>::AngleBetweenNormalized(const Vector& rhs) const
		{
			assert(IsNormalized() && rhs.IsNormalized() && "ERROR: Vector::AngleBetweenNormalized() was called for vectors which were not actually normalized!");

			if (std::is_constant_evaluated())
			{
				const float cosAngle = static_cast<float>(Dot(rhs));

				// std::acosf() is not yet constexpr, so we need to calculate the arccos(cosAngle) ourselves.
				// To do this, we instead use the formula arccos(x) = (PI / 2) - arcsin(x) and converge to the
				// arcsin(x) using its power series representation.
				//
				// You know, it isn't until you try to implement these functions yourself that you realize
				// why everybody hates transcendental functions in performance-critical code. Thankfully, these
				// calculations, slow as they may be, are all done at compile time.

				constexpr float MAXIMUM_ALLOWED_DIFFERENCE = 0.001f;
				float currArcSinValue = 0;
				float prevArcSinValue = currArcSinValue;
				std::size_t currIteration = 0;
				float currDifference = 0.0f;
				const float cosAngleSquared = (cosAngle * cosAngle);

				constexpr auto COMPUTE_FACTORIAL_LAMBDA = [] (const std::size_t value)
				{
					if (value <= 1)
						return 1;

					std::size_t currProduct = value;

					for (const auto i : std::views::iota(2u, value))
						currProduct *= i;

					return currProduct;
				};

				// The formula for the power series which represents arcsin(x) can be found at
				// https://en.wikipedia.org/wiki/Inverse_trigonometric_functions.
				do
				{
					float currIterationValue = cosAngle;
					const std::size_t twoTimesCurrIteration = (2 * currIteration);

					for (const auto i : std::views::iota(0u, currIteration))
						currArcSinValue *= (cosAngleSquared);

					currIterationValue /= static_cast<float>(twoTimesCurrIteration + 1);
					currIterationValue *= COMPUTE_FACTORIAL_LAMBDA(twoTimesCurrIteration);

					std::size_t scaleValueDenominator = ((static_cast<std::size_t>(1) << currIteration) * COMPUTE_FACTORIAL_LAMBDA(currIteration));
					scaleValueDenominator *= scaleValueDenominator;

					currIterationValue /= static_cast<float>(scaleValueDenominator);

					currArcSinValue += currIterationValue;
					currDifference = (currArcSinValue - prevArcSinValue);

					if (currDifference < 0.0f)
						currDifference *= -1.0f;

					prevArcSinValue = currArcSinValue;
				} while (currDifference > MAXIMUM_ALLOWED_DIFFERENCE);

				// We have (finally) converged to the correct result for arcsin(cosAngle), but we need arccos(cosAngle).
				// Thankfully, arccos(x) = (PI / 2) - arcsin(x).
				constexpr float PI = 3.1415926535f;
				constexpr float PI_OVER_TWO = (PI / 2.0f);

				return (PI_OVER_TWO - currArcSinValue);
			}
			else
			{
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredVector) };
				const MathType loadedRHS{ LOAD_FUNCTION(&(rhs.mStoredVector)) };

				MathType angleVector{};

				if constexpr (NumElements == 2)
					angleVector = DirectX::XMVector2AngleBetweenNormals(loadedLHS, loadedRHS);
				else if constexpr (NumElements == 3)
					angleVector = DirectX::XMVector3AngleBetweenNormals(loadedLHS, loadedRHS);
				else if constexpr (NumElements == 4)
					angleVector = DirectX::XMVector4AngleBetweenNormals(loadedLHS, loadedRHS);

				return DirectX::XMVectorGetX(angleVector);
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<float, NumElements> Vector<ElementType, NumElements>::GetReciprocal() const
		{
			if (std::is_constant_evaluated())
			{
				typename VectorInfo<float, NumElements>::StorageType storedResult{};
				storedResult.x = (1.0f / static_cast<float>(mStoredVector.x));
				storedResult.y = (1.0f / static_cast<float>(mStoredVector.y));

				if constexpr (NumElements >= 3)
					storedResult.z = (1.0f / static_cast<float>(mStoredVector.z));

				if constexpr (NumElements >= 4)
					storedResult.w = (1.0f / static_cast<float>(mStoredVector.w));

				return Vector<float, NumElements>{ storedResult };
			}
			else
			{
				const MathType loadedThis{ LOAD_FUNCTION(&mStoredVector) };
				const MathType reciprocalVector{ DirectX::XMVectorReciprocal(loadedThis) };

				typename VectorInfo<float, NumElements>::StorageType storedResult{};
				VectorInfo<float, NumElements>::STORE_FUNCTION(&storedResult, reciprocalVector);

				return Vector<float, NumElements>{ storedResult };
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr bool Vector<ElementType, NumElements>::IsNormalized() const
		{
			// A vector is considered normalized if its length is exactly equal to one.
			// The length of a vector is calculated as sqrt(x), where x is the sum of all of
			// its squared components. sqrt(x) == 1 if and only if x == 1. Thus, if we are
			// just checking if a vector is normalized or not, then we can skip the square root.

			if (std::is_constant_evaluated())
			{
				const ElementType currentSum = 0;
				currentSum += (mStoredVector.x * mStoredVector.x);
				currentSum += (mStoredVector.y * mStoredVector.y);

				if constexpr (NumElements >= 3)
					currentSum += (mStoredVector.z * mStoredVector.z);

				if constexpr (NumElements >= 4)
					currentSum += (mStoredVector.w * mStoredVector.w);

				if constexpr (std::is_same_v<ElementType, float>)
				{
					// For floating-point types, add an epsilon to the check.
					constexpr float EPSILON = 0.0001f;
					float sumDifference = (currentSum - 1.0f);

					if (sumDifference < 0.0f)
						sumDifference *= -1.0f;

					return (sumDifference < EPSILON);
				}
				else
					return (currentSum == 1);
			}
			else
			{
				const MathType loadedThis{ LOAD_FUNCTION(&mStoredVector) };
				MathType lengthSquaredVector{};

				if constexpr (NumElements == 2)
					lengthSquaredVector = DirectX::XMVector2LengthSq(loadedThis);
				else if constexpr (NumElements == 3)
					lengthSquaredVector = DirectX::XMVector3LengthSq(loadedThis);
				else if constexpr (NumElements == 4)
					lengthSquaredVector = DirectX::XMVector4LengthSq(loadedThis);

				const MathType lengthSquaredNearOneVector{ DirectX::XMVectorNearEqual(lengthSquaredVector, DirectX::XMVectorReplicate(1.0f), DirectX::XMVectorSplatEpsilon()) };
				return (DirectX::XMVectorGetIntX(lengthSquaredNearOneVector) != 0);
			}
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr ElementType Vector<ElementType, NumElements>::GetX() const
		{
			return mStoredVector.x;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr ElementType Vector<ElementType, NumElements>::GetY() const
		{
			return mStoredVector.y;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr ElementType Vector<ElementType, NumElements>::GetZ() const requires (NumElements >= 3)
		{
			return mStoredVector.z;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr ElementType Vector<ElementType, NumElements>::GetW() const requires (NumElements >= 4)
		{
			return mStoredVector.w;
		}

		template <typename ElementType, std::size_t NumElements>
		Vector<ElementType, NumElements>::MathType Vector<ElementType, NumElements>::GetDirectXMathVector() const
		{
			return LOAD_FUNCTION(&mStoredVector);
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements>& Vector<ElementType, NumElements>::operator+=(const Vector& rhs)
		{
			*this = AddVector(rhs);
			return *this;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements>& Vector<ElementType, NumElements>::operator-=(const Vector& rhs)
		{
			*this = SubtractVector(rhs);
			return *this;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements>& Vector<ElementType, NumElements>::operator*=(const Vector& rhs)
		{
			*this = PerComponentMultiplyVector(rhs);
			return *this;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements>& Vector<ElementType, NumElements>::operator/=(const Vector& rhs)
		{
			*this = PerComponentDivideVector(rhs);
			return *this;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements>& Vector<ElementType, NumElements>::operator+=(const ElementType rhs)
		{
			*this = AddScalar(rhs);
			return *this;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements>& Vector<ElementType, NumElements>::operator-=(const ElementType rhs)
		{
			*this = SubtractScalar(rhs);
			return *this;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements>& Vector<ElementType, NumElements>::operator*=(const ElementType rhs)
		{
			*this = MultiplyScalar(rhs);
			return *this;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements>& Vector<ElementType, NumElements>::operator/=(const ElementType rhs)
		{
			*this = DivideScalar(rhs);
			return *this;
		}

		template <typename ElementType, std::size_t NumElements>
		constexpr Vector<ElementType, NumElements>& Vector<ElementType, NumElements>::operator*=(const Matrix<NumElements, NumElements>& rhs)
		{
			*this = TransformByMatrix(rhs);
			return *this;
		}
	}
}

template <typename ElementType, std::size_t NumElements>
constexpr Brawler::Math::Vector<ElementType, NumElements> operator+(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const Brawler::Math::Vector<ElementType, NumElements>& rhs)
{
	return lhs.AddVector(rhs);
}

template <typename ElementType, std::size_t NumElements>
constexpr Brawler::Math::Vector<ElementType, NumElements> operator-(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const Brawler::Math::Vector<ElementType, NumElements>& rhs)
{
	return lhs.SubtractVector(rhs);
}

template <typename ElementType, std::size_t NumElements>
constexpr Brawler::Math::Vector<ElementType, NumElements> operator*(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const Brawler::Math::Vector<ElementType, NumElements>& rhs)
{
	return lhs.PerComponentMultiplyVector(rhs);
}

template <typename ElementType, std::size_t NumElements>
constexpr Brawler::Math::Vector<ElementType, NumElements> operator/(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const Brawler::Math::Vector<ElementType, NumElements>& rhs)
{
	return lhs.PerComponentDivideVector(rhs);
}

template <typename ElementType, std::size_t NumElements>
constexpr Brawler::Math::Vector<ElementType, NumElements> operator+(const Brawler::Math::Vector<ElementType, NumElements>& lhs, ElementType rhs)
{
	return lhs.AddScalar(rhs);
}

template <typename ElementType, std::size_t NumElements>
constexpr Brawler::Math::Vector<ElementType, NumElements> operator-(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const ElementType rhs)
{
	return lhs.SubtractScalar(rhs);
}

template <typename ElementType, std::size_t NumElements>
constexpr Brawler::Math::Vector<ElementType, NumElements> operator*(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const ElementType rhs)
{
	return lhs.MultiplyScalar(rhs);
}

template <typename ElementType, std::size_t NumElements>
constexpr Brawler::Math::Vector<ElementType, NumElements> operator/(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const ElementType rhs)
{
	return lhs.DivideScalar(rhs);
}

template <typename ElementType, std::size_t NumElements, std::size_t MatrixNumColumns>
constexpr Brawler::Math::Vector<ElementType, MatrixNumColumns> operator*(const Brawler::Math::Vector<ElementType, NumElements>& lhs, const Brawler::Math::Matrix<NumElements, MatrixNumColumns>& rhs)
{
	return lhs.TransformByMatrix(rhs);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace Math
	{
		using Int2 = Vector<std::int32_t, 2>;
		using UInt2 = Vector<std::uint32_t, 2>;
		using Float2 = Vector<float, 2>;

		using Int3 = Vector<std::int32_t, 3>;
		using UInt3 = Vector<std::uint32_t, 3>;
		using Float3 = Vector<float, 3>;

		using Int4 = Vector<std::int32_t, 4>;
		using UInt4 = Vector<std::uint32_t, 4>;
		using Float4 = Vector<float, 4>;
	}
}
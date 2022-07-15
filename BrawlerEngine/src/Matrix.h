#pragma once
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <array>
#include <ranges>
#include <DirectXMath/DirectXMath.h>

namespace Brawler
{
	namespace Math
	{
		template <std::size_t NumRows, std::size_t NumColumns>
		struct MatrixInfo
		{
			static_assert(sizeof(NumRows) != sizeof(NumRows));
		};

		template <
			typename StorageType_,
			auto LoadFunctionPtr,
			auto StoreFunctionPtr
		>
		struct MatrixInfoInstantiation
		{
			using StorageType = StorageType_;
			using MathType = DirectX::XMMATRIX;

			static constexpr auto LOAD_FUNCTION{ LoadFunctionPtr };
			static constexpr auto STORE_FUNCTION{ StoreFunctionPtr };
		};

		template <>
		struct MatrixInfo<3, 3> : public MatrixInfoInstantiation<DirectX::XMFLOAT3X3, DirectX::XMLoadFloat3x3, DirectX::XMStoreFloat3x3>
		{};

		template <>
		struct MatrixInfo<3, 4> : public MatrixInfoInstantiation<DirectX::XMFLOAT3X4, DirectX::XMLoadFloat3x4, DirectX::XMStoreFloat3x4>
		{};

		template <>
		struct MatrixInfo<4, 3> : public MatrixInfoInstantiation<DirectX::XMFLOAT4X3, DirectX::XMLoadFloat4x3, DirectX::XMStoreFloat4x3>
		{};

		template <>
		struct MatrixInfo<4, 4> : public MatrixInfoInstantiation<DirectX::XMFLOAT4X4, DirectX::XMLoadFloat4x4, DirectX::XMStoreFloat4x4>
		{};
	}
}

namespace Brawler
{
	namespace Math
	{
		template <std::size_t NumRows, std::size_t NumColumns>
		class Matrix
		{
		private:
			template <std::size_t RHSNumRows, std::size_t RHSNumColumns>
			friend class Matrix;

		private:
			using StorageType = typename MatrixInfo<NumRows, NumColumns>::StorageType;
			using MathType = typename MatrixInfo<NumRows, NumColumns>::MathType;

		public:
			constexpr Matrix() = default;
			constexpr explicit Matrix(const float replicatedScalar);

			// So, C++ has this stupid rule where you cannot switch the active member of a union in a constant-
			// evaluated context. Typically, when one initializes a DirectX::XMFLOAT*X* structure, they do so
			// by providing a set of floats. These structs have a constructor which takes all of these floats,
			// but they initialize the _XX members of the anonymous union contained within these structs.
			//
			// As a result, the _XX members become the active members of matrixData. If we then invoke the defaulted
			// trivial copy constructor of DirectX::XMFLOAT*X* to initialize mStoredMatrix from matrixData, then
			// the active members of mStoredMatrix will also be the _XX members. This poses a problem for us,
			// because our constexpr functions make use of the m member of the union.
			// 
			// If we create a constructor for Matrix which takes a const DirectX::XMFLOAT*X*& and manually copies
			// the values into the m member of mStoredMatrix, then it might work for those DirectX::XMFLOAT*X*
			// instances... but it will fail for default constructed DirectX::XMFLOAT*X* instances! Since there
			// is no way to check the active member of the union at runtime, we simply cannot allow construction
			// of Matrix instances from DirectX::XMFLOAT*X* structs.
			//
			// tl;dr: We can't add the constructor Matrix(const StorageType&).

		private:
			constexpr explicit Matrix(const StorageType& matrixData);

		public:
			constexpr explicit Matrix(
				const float _11, const float _12, const float _13,
				const float _21, const float _22, const float _23,
				const float _31, const float _32, const float _33
			) requires (NumRows == 3 && NumColumns == 3);

			constexpr explicit Matrix(
				const float _11, const float _12, const float _13, const float _14,
				const float _21, const float _22, const float _23, const float _24,
				const float _31, const float _32, const float _33, const float _34
			) requires (NumRows == 3 && NumColumns == 4);

			constexpr explicit Matrix(
				const float _11, const float _12, const float _13,
				const float _21, const float _22, const float _23,
				const float _31, const float _32, const float _33,
				const float _41, const float _42, const float _43
			) requires (NumRows == 4 && NumColumns == 3);

			constexpr explicit Matrix(
				const float _11, const float _12, const float _13, const float _14,
				const float _21, const float _22, const float _23, const float _24,
				const float _31, const float _32, const float _33, const float _34,
				const float _41, const float _42, const float _43, const float _44
			) requires (NumRows == 4 && NumColumns == 4);

			constexpr Matrix(const Matrix& rhs) = default;
			constexpr Matrix& operator=(const Matrix& rhs) = default;

			constexpr Matrix(Matrix&& rhs) noexcept = default;
			constexpr Matrix& operator=(Matrix&& rhs) noexcept = default;

			constexpr Matrix AddMatrix(const Matrix& rhs) const;
			constexpr Matrix SubtractMatrix(const Matrix& rhs) const;

			template <std::size_t RHSNumRows, std::size_t RHSNumColumns>
				requires (NumColumns == RHSNumRows)
			constexpr Matrix<NumRows, RHSNumColumns> MultiplyMatrix(const Matrix<RHSNumRows, RHSNumColumns>& rhs) const;

			constexpr Matrix AddScalar(const float rhs) const;
			constexpr Matrix SubtractScalar(const float rhs) const;
			constexpr Matrix MultiplyScalar(const float rhs) const;
			constexpr Matrix DivideScalar(const float rhs) const;

			constexpr Matrix Inverse() const requires (NumRows == NumColumns);
			constexpr Matrix<NumColumns, NumRows> Transpose() const;

			constexpr float Determinant() const requires (NumRows == NumColumns);

			/// <summary>
			/// Retrieves the element stored at row rowIndex and column columnIndex. In other words, let
			/// M represent an m x n matrix. Then, this function returns M[rowIndex][columnIndex]. The function
			/// asserts in Debug builds if either of rowIndex or columnIndex are out of the supported bounds.
			/// 
			/// NOTE: This function is a temporary means to access per-element Matrix data. Once the C++23
			/// multi-dimensional subscript operator is implemented in the MSVC (see C++ paper P2128R6), this
			/// function should be replaced and considered obsolete.
			/// </summary>
			/// <param name="rowIndex">
			/// - The row index at which the matrix will be searched.
			/// </param>
			/// <param name="columnIndex">
			/// - The column index at which the matrix will be searched.
			/// </param>
			/// <returns>
			/// The function returns the element stored at row rowIndex and column columnIndex.
			/// </returns>
			constexpr float& GetElement(const std::size_t rowIndex, const std::size_t columnIndex);

			constexpr float GetElement(const std::size_t rowIndex, const std::size_t columnIndex) const;

			MathType GetDirectXMathMatrix() const;

			template <std::size_t RHSNumRows, std::size_t RHSNumColumns>
				requires (NumColumns == RHSNumRows)
			constexpr Matrix<NumRows, RHSNumColumns> operator*(const Matrix<RHSNumRows, RHSNumColumns>& rhs) const;

			constexpr Matrix& operator+=(const Matrix& rhs);
			constexpr Matrix& operator-=(const Matrix& rhs);
			constexpr Matrix& operator*=(const Matrix& rhs) requires (NumRows == NumColumns);

			constexpr Matrix& operator+=(const float rhs);
			constexpr Matrix& operator-=(const float rhs);
			constexpr Matrix& operator*=(const float rhs);
			constexpr Matrix& operator/=(const float rhs);

		private:
			/// <summary>
			/// Calculates and returns the Matrix whose entries represent the cofactors of the
			/// transpose of this Matrix. The resulting matrix can be used in other calculations,
			/// such as for finding the determinant and finding the inverse of a matrix.
			/// 
			/// The function is marked consteval because there is no DirectXMath equivalent to
			/// this function, and because the transpose cofactor matrix is unlikely to be useful
			/// outside of calculating other matrices/values which already do have DirectXMath
			/// equivalents.
			/// </summary>
			/// <returns>
			/// The function returns the Matrix whose entries represent the cofactors of the
			/// transpose of this matrix.
			/// </returns>
			constexpr Matrix<NumRows, NumColumns> GetTransposeCofactorMatrix() const requires (NumRows == NumColumns);

			DirectX::XMVECTOR GetRowXMVector(const std::size_t rowIndex) const;

		private:
			StorageType mStoredMatrix;
		};
	}
}

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator+(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs);

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator-(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs);

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator+(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const float rhs);

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator+(const float lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs);

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator-(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const float rhs);

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator-(const float lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs);

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator*(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const float rhs);

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator*(const float lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs);

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator/(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const float rhs);

// Exclude operator/(const float lhs, const Matrix& rhs) because it makes no sense (i.e., scalar / matrix?).

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace Math
	{
		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>::Matrix(const float replicatedScalar) :
			mStoredMatrix()
		{
			for (const auto i : std::views::iota(0u, NumRows))
			{
				for (const auto j : std::views::iota(0u, NumColumns))
					mStoredMatrix.m[i][j] = replicatedScalar;
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>::Matrix(const StorageType& matrixData) :
			mStoredMatrix(matrixData)
		{}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>::Matrix(
			const float _11, const float _12, const float _13,
			const float _21, const float _22, const float _23,
			const float _31, const float _32, const float _33
		) requires (NumRows == 3 && NumColumns == 3)
		{
			mStoredMatrix.m[0][0] = _11;
			mStoredMatrix.m[0][1] = _12;
			mStoredMatrix.m[0][2] = _13;

			mStoredMatrix.m[1][0] = _21;
			mStoredMatrix.m[1][1] = _22;
			mStoredMatrix.m[1][2] = _23;

			mStoredMatrix.m[2][0] = _31;
			mStoredMatrix.m[2][1] = _32;
			mStoredMatrix.m[2][2] = _33;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>::Matrix(
			const float _11, const float _12, const float _13, const float _14,
			const float _21, const float _22, const float _23, const float _24,
			const float _31, const float _32, const float _33, const float _34
		) requires (NumRows == 3 && NumColumns == 4)
		{
			mStoredMatrix.m[0][0] = _11;
			mStoredMatrix.m[0][1] = _12;
			mStoredMatrix.m[0][2] = _13;
			mStoredMatrix.m[0][3] = _14;

			mStoredMatrix.m[1][0] = _21;
			mStoredMatrix.m[1][1] = _22;
			mStoredMatrix.m[1][2] = _23;
			mStoredMatrix.m[1][3] = _24;

			mStoredMatrix.m[2][0] = _31;
			mStoredMatrix.m[2][1] = _32;
			mStoredMatrix.m[2][2] = _33;
			mStoredMatrix.m[2][3] = _34;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>::Matrix(
			const float _11, const float _12, const float _13,
			const float _21, const float _22, const float _23,
			const float _31, const float _32, const float _33,
			const float _41, const float _42, const float _43
		) requires (NumRows == 4 && NumColumns == 3)
		{
			mStoredMatrix.m[0][0] = _11;
			mStoredMatrix.m[0][1] = _12;
			mStoredMatrix.m[0][2] = _13;

			mStoredMatrix.m[1][0] = _21;
			mStoredMatrix.m[1][1] = _22;
			mStoredMatrix.m[1][2] = _23;

			mStoredMatrix.m[2][0] = _31;
			mStoredMatrix.m[2][1] = _32;
			mStoredMatrix.m[2][2] = _33;

			mStoredMatrix.m[3][0] = _41;
			mStoredMatrix.m[3][1] = _42;
			mStoredMatrix.m[3][2] = _43;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>::Matrix(
			const float _11, const float _12, const float _13, const float _14,
			const float _21, const float _22, const float _23, const float _24,
			const float _31, const float _32, const float _33, const float _34,
			const float _41, const float _42, const float _43, const float _44
		) requires (NumRows == 4 && NumColumns == 4)
		{
			mStoredMatrix.m[0][0] = _11;
			mStoredMatrix.m[0][1] = _12;
			mStoredMatrix.m[0][2] = _13;
			mStoredMatrix.m[0][3] = _14;

			mStoredMatrix.m[1][0] = _21;
			mStoredMatrix.m[1][1] = _22;
			mStoredMatrix.m[1][2] = _23;
			mStoredMatrix.m[1][3] = _24;

			mStoredMatrix.m[2][0] = _31;
			mStoredMatrix.m[2][1] = _32;
			mStoredMatrix.m[2][2] = _33;
			mStoredMatrix.m[2][3] = _34;

			mStoredMatrix.m[3][0] = _41;
			mStoredMatrix.m[3][1] = _42;
			mStoredMatrix.m[3][2] = _43;
			mStoredMatrix.m[3][3] = _44;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns> Matrix<NumRows, NumColumns>::AddMatrix(const Matrix& rhs) const
		{
			if (std::is_constant_evaluated())
			{
				StorageType storedResult{};

				for (const auto i : std::views::iota(0u, NumRows))
				{
					for (const auto j : std::views::iota(0u, NumColumns))
						storedResult.m[i][j] = (mStoredMatrix.m[i][j] + rhs.mStoredMatrix.m[i][j]);
				}

				return Matrix{ storedResult };
			}
			else
			{
				// There is no DirectXMath equivalent. However, we can utilize DirectXMath vector
				// functions in order to vectorize the implementation ourselves.
				StorageType storedResult{};

				DirectX::XMVECTOR currLHSRow{};
				DirectX::XMVECTOR currRHSRow{};

				for (const auto i : std::views::iota(0u, NumRows))
				{
					currLHSRow = GetRowXMVector(i);
					currRHSRow = rhs.GetRowXMVector(i);

					const DirectX::XMVECTOR rowAddResult{ DirectX::XMVectorAdd(currLHSRow, currRHSRow) };

					storedResult.m[i][0] = DirectX::XMVectorGetX(rowAddResult);
					storedResult.m[i][1] = DirectX::XMVectorGetY(rowAddResult);
					storedResult.m[i][2] = DirectX::XMVectorGetZ(rowAddResult);

					if constexpr (NumColumns >= 4)
						storedResult.m[i][3] = DirectX::XMVectorGetW(rowAddResult);
				}

				return Matrix{ storedResult };
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns> Matrix<NumRows, NumColumns>::SubtractMatrix(const Matrix& rhs) const
		{
			if (std::is_constant_evaluated())
			{
				StorageType storedResult{};

				for (const auto i : std::views::iota(0u, NumRows))
				{
					for (const auto j : std::views::iota(0u, NumColumns))
						storedResult.m[i][j] = (mStoredMatrix.m[i][j] - rhs.mStoredMatrix.m[i][j]);
				}

				return Matrix{ storedResult };
			}
			else
			{
				// There is no DirectXMath equivalent. However, we can utilize DirectXMath vector
				// functions in order to vectorize the implementation ourselves.
				StorageType storedResult{};

				DirectX::XMVECTOR currLHSRow{};
				DirectX::XMVECTOR currRHSRow{};

				for (const auto i : std::views::iota(0u, NumRows))
				{
					currLHSRow = GetRowXMVector(i);
					currRHSRow = rhs.GetRowXMVector(i);

					const DirectX::XMVECTOR rowSubtractResult{ DirectX::XMVectorSubtract(currLHSRow, currRHSRow) };

					storedResult.m[i][0] = DirectX::XMVectorGetX(rowSubtractResult);
					storedResult.m[i][1] = DirectX::XMVectorGetY(rowSubtractResult);
					storedResult.m[i][2] = DirectX::XMVectorGetZ(rowSubtractResult);

					if constexpr (NumColumns >= 4)
						storedResult.m[i][3] = DirectX::XMVectorGetW(rowSubtractResult);
				}

				return Matrix{ storedResult };
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		template <std::size_t RHSNumRows, std::size_t RHSNumColumns>
			requires (NumColumns == RHSNumRows)
		constexpr Matrix<NumRows, RHSNumColumns> Matrix<NumRows, NumColumns>::MultiplyMatrix(const Matrix<RHSNumRows, RHSNumColumns>& rhs) const
		{
			if (std::is_constant_evaluated())
			{
				typename MatrixInfo<NumRows, RHSNumColumns>::StorageType storedResult{};

				for (const auto i : std::views::iota(0u, NumRows))
				{
					for (const auto j : std::views::iota(0u, RHSNumColumns))
					{
						float currDotProductValue = 0.0f;

						for (const auto k : std::views::iota(0u, NumColumns))
							currDotProductValue += (mStoredMatrix.m[i][k] * rhs.mStoredMatrix.m[k][j]);

						storedResult.m[i][j] = currDotProductValue;
					}
				}

				return Matrix<NumRows, RHSNumColumns>{ storedResult };
			}
			else
			{
				const MathType loadedLHS{ MatrixInfo<NumRows, NumColumns>::LOAD_FUNCTION(&mStoredMatrix) };
				const typename MatrixInfo<RHSNumRows, RHSNumColumns>::MathType loadedRHS{ MatrixInfo<RHSNumRows, RHSNumColumns>::LOAD_FUNCTION(&(rhs.mStoredMatrix)) };

				const typename MatrixInfo<NumRows, RHSNumColumns>::MathType multiplyResult{ DirectX::XMMatrixMultiply(loadedLHS, loadedRHS) };

				typename MatrixInfo<NumRows, RHSNumColumns>::StorageType storedResult{};
				MatrixInfo<NumRows, RHSNumColumns>::STORE_FUNCTION(&storedResult, multiplyResult);

				return Matrix<NumRows, RHSNumColumns>{ storedResult };
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns> Matrix<NumRows, NumColumns>::AddScalar(const float rhs) const
		{
			if (std::is_constant_evaluated())
			{
				StorageType storedResult{};

				for (const auto i : std::views::iota(0u, NumRows))
				{
					for (const auto j : std::views::iota(0u, NumColumns))
						storedResult.m[i][j] = (mStoredMatrix.m[i][j] + rhs);
				}

				return Matrix<NumRows, NumColumns>{ storedResult };
			}
			else
			{
				// There is no DirectXMath equivalent. However, we can utilize DirectXMath vector
				// functions in order to vectorize the implementation ourselves.
				StorageType storedResult{};
				const DirectX::XMVECTOR rhsVector{ DirectX::XMVectorReplicate(rhs) };

				for (const auto i : std::views::iota(0u, NumRows))
				{
					const DirectX::XMVECTOR lhsVector{ GetRowXMVector(i) };

					const DirectX::XMVECTOR currRowAddResult{ DirectX::XMVectorAdd(lhsVector, rhsVector) };

					storedResult.m[i][0] = DirectX::XMVectorGetX(currRowAddResult);
					storedResult.m[i][1] = DirectX::XMVectorGetY(currRowAddResult);
					storedResult.m[i][2] = DirectX::XMVectorGetZ(currRowAddResult);

					if constexpr (NumColumns >= 4)
						storedResult.m[i][3] = DirectX::XMVectorGetW(currRowAddResult);
				}

				return Matrix<NumRows, NumColumns>{ storedResult };
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns> Matrix<NumRows, NumColumns>::SubtractScalar(const float rhs) const
		{
			if (std::is_constant_evaluated())
			{
				StorageType storedResult{};

				for (const auto i : std::views::iota(0u, NumRows))
				{
					for (const auto j : std::views::iota(0u, NumColumns))
						storedResult.m[i][j] = (mStoredMatrix.m[i][j] - rhs);
				}

				return Matrix<NumRows, NumColumns>{ storedResult };
			}
			else
			{
				// There is no DirectXMath equivalent. However, we can utilize DirectXMath vector
				// functions in order to vectorize the implementation ourselves.
				StorageType storedResult{};
				const DirectX::XMVECTOR rhsVector{ DirectX::XMVectorReplicate(rhs) };

				for (const auto i : std::views::iota(0u, NumRows))
				{
					const DirectX::XMVECTOR lhsVector{ GetRowXMVector(i) };

					const DirectX::XMVECTOR currRowSubtractResult{ DirectX::XMVectorSubtract(lhsVector, rhsVector) };

					storedResult.m[i][0] = DirectX::XMVectorGetX(currRowSubtractResult);
					storedResult.m[i][1] = DirectX::XMVectorGetY(currRowSubtractResult);
					storedResult.m[i][2] = DirectX::XMVectorGetZ(currRowSubtractResult);

					if constexpr (NumColumns >= 4)
						storedResult.m[i][3] = DirectX::XMVectorGetW(currRowSubtractResult);
				}

				return Matrix<NumRows, NumColumns>{ storedResult };
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns> Matrix<NumRows, NumColumns>::MultiplyScalar(const float rhs) const
		{
			if (std::is_constant_evaluated())
			{
				StorageType storedResult{};

				for (const auto i : std::views::iota(0u, NumRows))
				{
					for (const auto j : std::views::iota(0u, NumColumns))
						storedResult.m[i][j] = (mStoredMatrix.m[i][j] * rhs);
				}

				return Matrix<NumRows, NumColumns>{ storedResult };
			}
			else
			{
				// There is no DirectXMath equivalent. However, we can utilize DirectXMath vector
				// functions in order to vectorize the implementation ourselves.
				StorageType storedResult{};

				for (const auto i : std::views::iota(0u, NumRows))
				{
					const DirectX::XMVECTOR lhsVector{ GetRowXMVector(i) };

					const DirectX::XMVECTOR currRowMultiplyResult{ DirectX::XMVectorScale(lhsVector, rhs) };

					storedResult.m[i][0] = DirectX::XMVectorGetX(currRowMultiplyResult);
					storedResult.m[i][1] = DirectX::XMVectorGetY(currRowMultiplyResult);
					storedResult.m[i][2] = DirectX::XMVectorGetZ(currRowMultiplyResult);

					if constexpr (NumColumns >= 4)
						storedResult.m[i][3] = DirectX::XMVectorGetW(currRowMultiplyResult);
				}

				return Matrix<NumRows, NumColumns>{ storedResult };
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns> Matrix<NumRows, NumColumns>::DivideScalar(const float rhs) const
		{
			constexpr float EPSILON = 0.0001f;

			const float absRHS = (rhs < 0.0f ? -rhs : rhs);
			assert(absRHS >= EPSILON && "ERROR: An attempt was made to divide the elements of a Matrix by zero!");

			if (std::is_constant_evaluated())
			{
				StorageType storedResult{};

				for (const auto i : std::views::iota(0u, NumRows))
				{
					for (const auto j : std::views::iota(0u, NumColumns))
						storedResult.m[i][j] = (mStoredMatrix.m[i][j] / rhs);
				}

				return Matrix<NumRows, NumColumns>{ storedResult };
			}
			else
			{
				// There is no DirectXMath equivalent. However, we can utilize DirectXMath vector
				// functions in order to vectorize the implementation ourselves.
				StorageType storedResult{};
				const float reciprocalRHS = (1.0f / rhs);

				for (const auto i : std::views::iota(0u, NumRows))
				{
					const DirectX::XMVECTOR lhsVector{ GetRowXMVector(i) };

					const DirectX::XMVECTOR currRowDivideResult{ DirectX::XMVectorScale(lhsVector, reciprocalRHS) };

					storedResult.m[i][0] = DirectX::XMVectorGetX(currRowDivideResult);
					storedResult.m[i][1] = DirectX::XMVectorGetY(currRowDivideResult);
					storedResult.m[i][2] = DirectX::XMVectorGetZ(currRowDivideResult);

					if constexpr (NumColumns >= 4)
						storedResult.m[i][3] = DirectX::XMVectorGetW(currRowDivideResult);
				}

				return Matrix<NumRows, NumColumns>{ storedResult };
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns> Matrix<NumRows, NumColumns>::Inverse() const requires (NumRows == NumColumns)
		{
			if (std::is_constant_evaluated())
			{
				// Yeah... This is gonna be slow. Well, it's not like compile times were all that great
				// to begin with, anyways.
				const Matrix<NumRows, NumColumns> transposeCofactorMatrix{ GetTransposeCofactorMatrix() };

				// Since we have the transpose cofactor matrix, we can calculate the determinant quickly
				// by using it, rather than just calling Determinant().

				float determinant = 0.0f;

				for (const auto i : std::views::iota(0u, NumRows))
					determinant += (GetElement(0, i) * transposeCofactorMatrix.GetElement(i, 0));

				constexpr float SINGULARITY_EPSILON = 0.0001f;
				const float absDeterminant = (determinant < 0.0f ? -determinant : determinant);

				assert(absDeterminant > SINGULARITY_EPSILON && "ERROR: Matrix::Inverse() was called for a singular (i.e., non-invertible) matrix!");

				return transposeCofactorMatrix / determinant;
			}
			else
			{
				const MathType loadedThis{ MatrixInfo<NumRows, NumColumns>::LOAD_FUNCTION(&mStoredMatrix) };
				const MathType inverseMatrix{ DirectX::XMMatrixInverse(nullptr, loadedThis) };

				assert(!DirectX::XMMatrixIsInfinite(inverseMatrix) && "ERROR: Matrix::Inverse() was called for a singular (i.e., non-invertible) matrix!");

				StorageType storedResult{};
				MatrixInfo<NumRows, NumColumns>::STORE_FUNCTION(&storedResult, inverseMatrix);

				return Matrix{ storedResult };
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumColumns, NumRows> Matrix<NumRows, NumColumns>::Transpose() const
		{
			if (std::is_constant_evaluated())
			{
				typename MatrixInfo<NumColumns, NumRows>::StorageType storedResult{};

				for (const auto i : std::views::iota(0u, NumColumns))
				{
					for (const auto j : std::views::iota(0u, NumRows))
						storedResult.m[i][j] = mStoredMatrix.m[j][i];
				}

				return Matrix<NumColumns, NumRows>{ storedResult };
			}
			else
			{
				const MathType loadedThis{ MatrixInfo<NumRows, NumColumns>::LOAD_FUNCTION(&mStoredMatrix) };
				const MathType transposedMatrix{ DirectX::XMMatrixTranspose(loadedThis) };

				typename MatrixInfo<NumColumns, NumRows>::StorageType storedResult{};
				MatrixInfo<NumColumns, NumRows>::STORE_FUNCTION(&storedResult, transposedMatrix);

				return Matrix<NumColumns, NumRows>{ storedResult };
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr float Matrix<NumRows, NumColumns>::Determinant() const requires (NumRows == NumColumns)
		{
			if (std::is_constant_evaluated())
			{
				// Since we only support matrices with at least 3 rows and 3 columns (since that is what DirectXMath
				// supports), we use NumRows == 3 as the base case. Yes, we are using recursion here, but it's important
				// to note that this code path is only taken at compile time. At runtime, we use the much more efficient
				// vectorized DirectXMath library.
				if constexpr (NumRows == 3)
				{
					const float firstProduct = mStoredMatrix.m[0][0] * ((mStoredMatrix.m[1][1] * mStoredMatrix.m[2][2]) - (mStoredMatrix.m[1][2] * mStoredMatrix.m[2][1]));
					const float secondProduct = mStoredMatrix.m[0][1] * ((mStoredMatrix.m[1][0] * mStoredMatrix.m[2][2]) - (mStoredMatrix.m[1][2] * mStoredMatrix.m[2][0]));
					const float thirdProduct = mStoredMatrix.m[0][2] * ((mStoredMatrix.m[1][0] * mStoredMatrix.m[2][1]) - (mStoredMatrix.m[1][1] * mStoredMatrix.m[2][0]));

					return firstProduct - secondProduct + thirdProduct;
				}
				else
				{
					float currDeterminantValue = 0.0f;

					for (const auto i : std::views::iota(0u, NumColumns))
					{
						typename MatrixInfo<(NumRows - 1), (NumColumns - 1)>::StorageType storedSubMatrix{};
						const std::size_t indexToSkip = i;
						const auto transformIndexLambda = [indexToSkip] (const std::size_t currIndex)
						{
							return (currIndex >= indexToSkip ? (currIndex + 1) : currIndex);
						};

						for (const auto currRow : std::views::iota(0u, (NumRows - 1)))
						{
							for (const auto currColumn : std::views::iota(0u, (NumColumns - 1)))
							{
								const std::size_t copyRowIndex = transformIndexLambda(currRow);
								const std::size_t copyColumnIndex = transformIndexLambda(currColumn);

								storedSubMatrix.m[currRow][currColumn] = mStoredMatrix.m[copyRowIndex][copyColumnIndex];
							}
						}

						const float subDeterminant = (-1.0f * (i % 2)) * Matrix<(NumRows - 1), (NumColumns - 1)>{ storedSubMatrix }.Determinant();
						currDeterminantValue += subDeterminant;
					}

					return currDeterminantValue;
				}
			}
			else
			{
				const MathType loadedThis{ MatrixInfo<NumRows, NumColumns>::LOAD_FUNCTION(&mStoredMatrix) };
				const DirectX::XMVECTOR determinantVector{ DirectX::XMMatrixDeterminant(loadedThis) };

				return DirectX::XMVectorGetX(determinantVector);
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr float& Matrix<NumRows, NumColumns>::GetElement(const std::size_t rowIndex, const std::size_t columnIndex)
		{
			assert(rowIndex < NumRows&& columnIndex < NumColumns && "ERROR: An out-of-bounds index was specified in a call to Matrix::GetElement()!");
			return mStoredMatrix.m[rowIndex][columnIndex];
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr float Matrix<NumRows, NumColumns>::GetElement(const std::size_t rowIndex, const std::size_t columnIndex) const
		{
			assert(rowIndex < NumRows && columnIndex < NumColumns && "ERROR: An out-of-bounds index was specified in a call to Matrix::GetElement()!");
			return mStoredMatrix.m[rowIndex][columnIndex];
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		Matrix<NumRows, NumColumns>::MathType Matrix<NumRows, NumColumns>::GetDirectXMathMatrix() const
		{
			return MatrixInfo<NumRows, NumColumns>::LOAD_FUNCTION(&mStoredMatrix);
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		template <std::size_t RHSNumRows, std::size_t RHSNumColumns>
			requires (NumColumns == RHSNumRows)
		constexpr Matrix<NumRows, RHSNumColumns> Matrix<NumRows, NumColumns>::operator*(const Matrix<RHSNumRows, RHSNumColumns>& rhs) const
		{
			return MultiplyMatrix(rhs);
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>& Matrix<NumRows, NumColumns>::operator+=(const Matrix& rhs)
		{
			*this = AddMatrix(rhs);
			return *this;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>& Matrix<NumRows, NumColumns>::operator-=(const Matrix& rhs)
		{
			*this = SubtractMatrix(rhs);
			return *this;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>& Matrix<NumRows, NumColumns>::operator*=(const Matrix& rhs) requires (NumRows == NumColumns)
		{
			*this = MultiplyMatrix(rhs);
			return *this;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>& Matrix<NumRows, NumColumns>::operator+=(const float rhs)
		{
			*this = AddScalar(rhs);
			return *this;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>& Matrix<NumRows, NumColumns>::operator-=(const float rhs)
		{
			*this = SubtractScalar(rhs);
			return *this;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>& Matrix<NumRows, NumColumns>::operator*=(const float rhs)
		{
			*this = MultiplyScalar(rhs);
			return *this;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>& Matrix<NumRows, NumColumns>::operator/=(const float rhs)
		{
			*this = DivideScalar(rhs);
			return *this;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns> Matrix<NumRows, NumColumns>::GetTransposeCofactorMatrix() const requires (NumRows == NumColumns)
		{
			const Matrix<NumRows, NumColumns> transposedMatrix{ Transpose() };
			StorageType storageTransposeCofactorMatrix{};

			for (const auto currRow : std::views::iota(0u, NumRows))
			{
				for (const auto currColumn : std::views::iota(0u, NumColumns))
				{
					const float negationScalar = (((currRow + currColumn) % 2) == 0 ? 1.0f : -1.0f);

					if constexpr (NumRows > 3 && NumColumns > 3)
					{
						typename MatrixInfo<(NumRows - 1), (NumColumns - 1)>::StorageType currStorageDeterminantMatrix{};

						for (const auto determinantMatrixRow : std::views::iota(0u, (NumRows - 1)))
						{
							const std::size_t rowIndexToCopy = (determinantMatrixRow >= currRow ? (determinantMatrixRow + 1) : determinantMatrixRow);

							for (const auto determinantMatrixColumn : std::views::iota(0u, NumColumns - 1))
							{
								const std::size_t columnIndexToCopy = (determinantMatrixColumn >= currColumn ? (determinantMatrixColumn + 1) : determinantMatrixColumn);

								currStorageDeterminantMatrix.m[determinantMatrixRow][determinantMatrixColumn] = transposedMatrix.GetElement(rowIndexToCopy, columnIndexToCopy);
							}
						}

						Matrix<(NumRows - 1), (NumColumns - 1)> currDeterminantMatrix{};
						currDeterminantMatrix.mStoredMatrix = currStorageDeterminantMatrix;

						storageTransposeCofactorMatrix.m[currRow][currColumn] = (negationScalar * currDeterminantMatrix.Determinant());
					}
					else
					{
						struct Matrix2x2
						{
							std::array<std::array<float, 2>, 2> DataArr;
						};

						Matrix2x2 currStorageDeterminantMatrix{};

						for (const auto determinantMatrixRow : std::views::iota(0u, (NumRows - 1)))
						{
							const std::size_t rowIndexToCopy = (determinantMatrixRow >= currRow ? (determinantMatrixRow + 1) : determinantMatrixRow);

							for (const auto determinantMatrixColumn : std::views::iota(0u, (NumColumns - 1)))
							{
								const std::size_t columnIndexToCopy = (determinantMatrixColumn >= currColumn ? (determinantMatrixColumn + 1) : determinantMatrixColumn);

								currStorageDeterminantMatrix.DataArr[determinantMatrixRow][determinantMatrixColumn] = transposedMatrix.GetElement(rowIndexToCopy, columnIndexToCopy);
							}
						}

						const float currDeterminant = ((currStorageDeterminantMatrix.DataArr[0][0] * currStorageDeterminantMatrix.DataArr[1][1]) - (currStorageDeterminantMatrix.DataArr[0][1] * currStorageDeterminantMatrix.DataArr[1][0]));
						storageTransposeCofactorMatrix.m[currRow][currColumn] = (negationScalar * currDeterminant);
					}
				}
			}

			return Matrix<NumRows, NumColumns>{ storageTransposeCofactorMatrix };
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		DirectX::XMVECTOR Matrix<NumRows, NumColumns>::GetRowXMVector(const std::size_t rowIndex) const
		{
			assert(rowIndex < NumRows);

			if constexpr (NumColumns == 3)
			{
				return DirectX::XMVectorSet(
					mStoredMatrix.m[rowIndex][0],
					mStoredMatrix.m[rowIndex][1],
					mStoredMatrix.m[rowIndex][2],
					0.0f
				);
			}
			else if constexpr (NumColumns == 4)
			{
				return DirectX::XMVectorSet(
					mStoredMatrix.m[rowIndex][0],
					mStoredMatrix.m[rowIndex][1],
					mStoredMatrix.m[rowIndex][2],
					mStoredMatrix.m[rowIndex][3]
				);
			}
			else
			{
				assert(false && "ERROR: A Matrix instance which did not have the appropriate number of elements to create a DirectX::XMVECTOR instance from one of its rows attempted to create one anyways!");
				return DirectX::XMVectorZero();
			}
		}
	}
}

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator+(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs)
{
	return lhs.AddMatrix(rhs);
}

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator-(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs)
{
	return lhs.SubtractMatrix(rhs);
}

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator+(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const float rhs)
{
	return lhs.AddScalar(rhs);
}

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator+(const float lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs)
{
	// a + b = b + a
	return rhs.AddScalar(lhs);
}

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator-(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const float rhs)
{
	return lhs.SubtractScalar(rhs);
}

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator-(const float lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs)
{
	// a - b = (-b) + a
	return rhs.MultiplyScalar(-1.0f).AddScalar(lhs);
}

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator*(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const float rhs)
{
	return lhs.MultiplyScalar(rhs);
}

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator*(const float lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs)
{
	// a * b = b * a (Well, for scalar-matrix multiplication, anyways...)
	return rhs.MultiplyScalar(lhs);
}

template <std::size_t NumRows, std::size_t NumColumns>
constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator/(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const float rhs)
{
	return lhs.DivideScalar(rhs);
}
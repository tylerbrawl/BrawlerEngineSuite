module;
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <ranges>
#include <DirectXMath/DirectXMath.h>

export module Brawler.Math.MathTypes:Matrix;

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
			using StorageType = typename MatrixInfo<NumRows, NumColumns>::StorageType;
			using MathType = typename MatrixInfo<NumRows, NumColumns>::MathType;

			static constexpr auto LOAD_FUNCTION{ MatrixInfo<NumRows, NumColumns>::LOAD_FUNCTION };
			static constexpr auto STORE_FUNCTION{ MatrixInfo<NumRows, NumColumns>::STORE_FUNCTION };

		public:
			constexpr Matrix() = default;
			constexpr explicit Matrix(const StorageType& matrixData);
			constexpr explicit Matrix(const float replicatedScalar);

			constexpr Matrix(const Matrix& rhs) = default;
			constexpr Matrix& operator=(const Matrix& rhs) = default;

			constexpr Matrix(Matrix&& rhs) noexcept = default;
			constexpr Matrix& operator=(Matrix&& rhs) noexcept = default;

			constexpr Matrix AddMatrix(const Matrix& rhs) const;
			constexpr Matrix SubtractMatrix(const Matrix& rhs) const;

			template <std::size_t RHSNumRows, std::size_t RHSNumColumns>
				requires (NumColumns == RHSNumRows)
			constexpr Matrix<NumRows, RHSNumColumns> MultiplyMatrix(const Matrix<RHSNumRows, RHSNumColumns>& rhs) const;

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
			constexpr float GetElement(const std::size_t rowIndex, const std::size_t columnIndex) const;

			MathType GetDirectXMathMatrix() const;

			constexpr Matrix& operator+=(const Matrix& rhs);
			constexpr Matrix& operator-=(const Matrix& rhs);
			constexpr Matrix& operator*=(const Matrix& rhs) requires (NumRows == NumColumns);

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
			consteval Matrix<NumRows, NumColumns> GetTransposeCofactorMatrix() const requires (NumRows == NumColumns);

			DirectX::XMVECTOR GetRowXMVector(const std::size_t rowIndex) const;

		private:
			StorageType mStoredMatrix;
		};
	}
}

export
{
	template <std::size_t NumRows, std::size_t NumColumns>
	constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator+(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs);

	template <std::size_t NumRows, std::size_t NumColumns>
	constexpr Brawler::Math::Matrix<NumRows, NumColumns> operator-(const Brawler::Math::Matrix<NumRows, NumColumns>& lhs, const Brawler::Math::Matrix<NumRows, NumColumns>& rhs);

	template <std::size_t LHSNumRows, std::size_t LHSNumColumns, std::size_t RHSNumRows, std::size_t RHSNumColumns>
		requires (LHSNumColumns == RHSNumRows)
	constexpr Brawler::Math::Matrix<LHSNumRows, RHSNumColumns> operator*(const Brawler::Math::Matrix<LHSNumRows, LHSNumColumns>& lhs, const Brawler::Math::Matrix<RHSNumRows, RHSNumColumns>& rhs);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace Math
	{
		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr Matrix<NumRows, NumColumns>::Matrix(const StorageType& matrixData) :
			mStoredMatrix(matrixData)
		{}

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
				const MathType loadedLHS{ LOAD_FUNCTION(&mStoredMatrix) };
				const typename MatrixInfo<RHSNumRows, RHSNumColumns>::MathType loadedRHS{ MatrixInfo<RHSNumRows, RHSNumColumns>::LOAD_FUNCTION(&(rhs.mStoredMatrix)) };

				const typename MatrixInfo<NumRows, RHSNumColumns>::MathType multiplyResult{ DirectX::XMMatrixMultiply(loadedLHS, loadedRHS) };

				typename MatrixInfo<NumRows, RHSNumColumns>::StorageType storedResult{};
				MatrixInfo<NumRows, RHSNumColumns>::STORE_FUNCTION(&storedResult, multiplyResult);

				return Matrix<NumRows, RHSNumColumns>{ storedResult };
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

			}
			else
			{
				const MathType loadedThis{ LOAD_FUNCTION(&mStoredMatrix) };
				const MathType inverseMatrix{ DirectX::XMMatrixInverse(nullptr, loadedThis) };

				assert(!DirectX::XMMatrixIsInfinite(inverseMatrix) && "ERROR: Matrix::Inverse() was called for a singular (i.e., non-invertible) matrix!");

				StorageType storedResult{};
				STORE_FUNCTION(&storedResult, inverseMatrix);

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
				const MathType loadedThis{ LOAD_FUNCTION(&mStoredMatrix) };
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
					const float firstProduct = mStoredMatrix._11 * ((mStoredMatrix._22 * mStoredMatrix._33) - (mStoredMatrix._23 * mStoredMatrix._32));
					const float secondProduct = mStoredMatrix._12 * ((mStoredMatrix._21 * mStoredMatrix._33) - (mStoredMatrix._23 * mStoredMatrix._31));
					const float thirdProduct = mStoredMatrix._13 * ((mStoredMatrix._21 * mStoredMatrix._32) - (mStoredMatrix._22 * mStoredMatrix._31));

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
				const MathType loadedThis{ LOAD_FUNCTION(&mStoredMatrix) };
				const DirectX::XMVECTOR determinantVector{ DirectX::XMMatrixDeterminant(loadedThis) };

				return DirectX::XMVectorGetX(determinantVector);
			}
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		constexpr float Matrix<NumRows, NumColumns>::GetElement(const std::size_t rowIndex, const std::size_t columnIndex) const
		{
			assert(rowIndex < NumRows&& columnIndex < NumColumns && "ERROR: An out-of-bounds index was specified in a call to Matrix::GetElement()!");
			return mStoredMatrix.m[rowIndex][columnIndex];
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		Matrix<NumRows, NumColumns>::MathType Matrix<NumRows, NumColumns>::GetDirectXMathMatrix() const
		{
			return LOAD_FUNCTION(&mStoredMatrix);
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
		consteval Matrix<NumRows, NumColumns> Matrix<NumRows, NumColumns>::GetTransposeCofactorMatrix() const requires (NumRows == NumColumns)
		{
			const Matrix<NumRows, NumColumns> transposedMatrix{ Transpose() };
			StorageType storageTransposeCofactorMatrix{};

			for (const auto currRow : std::views::iota(0u, NumRows))
			{
				for (const auto currColumn : std::views::iota(0u, NumColumns))
				{
					const float negationScalar = (-1.0f * ((currRow + currColumn) % 2));

					typename MatrixInfo<(NumRows - 1), (NumColumns - 1)>::StorageType currStorageDeterminantMatrix{};

					for (const auto determinantMatrixRow : std::views::iota(0u, (NumRows - 1)))
					{
						const std::size_t rowIndexToCopy = (determinantMatrixRow >= currRow ? (determinantMatrixRow + 1) : determinantMatrixRow);

						for (const auto determinantMatrixColumn : std::views::iota(NumColumns - 1))
						{
							const std::size_t columnIndexToCopy = (determinantMatrixColumn >= currColumn ? (determinantMatrixColumn + 1) : determinantMatrixColumn);

							currStorageDeterminantMatrix[determinantMatrixRow][determinantMatrixColumn] = transposedMatrix.GetElement(rowIndexToCopy, columnIndexToCopy);
						}
					}

					Matrix<(NumRows - 1), (NumColumns - 1)> currDeterminantMatrix{ currStorageDeterminantMatrix };
					storageTransposeCofactorMatrix[currRow][currColumn] = (negationScalar * currDeterminantMatrix.Determinant());
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

template <std::size_t LHSNumRows, std::size_t LHSNumColumns, std::size_t RHSNumRows, std::size_t RHSNumColumns>
	requires (LHSNumColumns == RHSNumRows)
constexpr Brawler::Math::Matrix<LHSNumRows, RHSNumColumns> operator*(const Brawler::Math::Matrix<LHSNumRows, LHSNumColumns>& lhs, const Brawler::Math::Matrix<RHSNumRows, RHSNumColumns>& rhs)
{
	return lhs.MultiplyMatrix(rhs);
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	namespace Math
	{
		using Float3x3 = Matrix<3, 3>;
		using Float3x4 = Matrix<3, 4>;
		using Float4x3 = Matrix<4, 3>;
		using Float4x4 = Matrix<4, 4>;
	}
}
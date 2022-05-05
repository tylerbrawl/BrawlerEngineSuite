module;
#include <cstddef>
#include <optional>
#include <assimp/matrix4x4.h>
#include <DirectXMath/DirectXMath.h>

export module Brawler.Math.Matrix;

namespace
{
	template <typename RetType, typename... Args>
	using DirectXMathFunctionPtr = RetType(XM_CALLCONV *)(Args...);
}

// Pro Tip: If you are getting error C2653 using modules and forward declarations, make
// sure that your forward declaration is also exported, like so:

export namespace Brawler
{
	namespace Math
	{
		template <std::size_t NumRows, std::size_t NumColumns>
		class Matrix;
	}
}

export
{
	template <std::size_t M1Rows, std::size_t M1Columns, std::size_t M2Rows, std::size_t M2Columns>
		requires (M1Columns == M2Rows)
	Brawler::Math::Matrix<M1Rows, M2Columns> operator*(const Brawler::Math::Matrix<M1Rows, M1Columns>& lhs, const Brawler::Math::Matrix<M2Rows, M2Columns>& rhs);
}

namespace Brawler
{
	namespace Math
	{
		namespace IMPL
		{
			template <typename DirectXMatrixType>
			using MatrixStoreFunction_T = DirectXMathFunctionPtr<void, DirectXMatrixType*, DirectX::XMMATRIX>;

			template <typename DirectXMatrixType>
			using MatrixLoadFunction_T = DirectXMathFunctionPtr<DirectX::XMMATRIX, const DirectXMatrixType*>;

			template <
				typename DirectXMatrixType_,
				typename AssimpMatrixType_,
				MatrixStoreFunction_T<DirectXMatrixType_> StoreFunction_,
				MatrixLoadFunction_T<DirectXMatrixType_> LoadFunction_
			>
				requires requires (DirectXMatrixType_ dxMatrix, AssimpMatrixType_ aiMatrix)
			{
				// A valid DirectXMatrixType_ type can be used in a DirectXMath loading function.
				LoadFunction_(&dxMatrix);

				// A valid AssimpMatrixType_ has the member function Transpose().
				aiMatrix.Transpose();
			}
				struct MatrixInfoInstantiation
			{
				using DirectXMatrixType = DirectXMatrixType_;
				using AssimpMatrixType = AssimpMatrixType_;

				static constexpr MatrixStoreFunction_T<DirectXMatrixType> StoreFunction = StoreFunction_;
				static constexpr MatrixLoadFunction_T<DirectXMatrixType> LoadFunction = LoadFunction_;
			};

			template <std::size_t NumRows, std::size_t NumColumns>
			struct MatrixInfo
			{
				static_assert(sizeof(NumRows) != sizeof(NumRows), "ERROR: An explicit template specialization for Brawler::Math::IMPL::MatrixInfo was never specified for this combination of rows and columns! (See Matrix.ixx.)");
			};

			template <>
			struct MatrixInfo<4, 4> : public MatrixInfoInstantiation<
				DirectX::XMFLOAT4X4,
				aiMatrix4x4,
				DirectX::XMStoreFloat4x4,
				DirectX::XMLoadFloat4x4
			>
			{};
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		class Matrix
		{
		private:
			using MatInfo = IMPL::MatrixInfo<NumRows, NumColumns>;

		private:
			template <std::size_t M1Rows, std::size_t M1Columns, std::size_t M2Rows, std::size_t M2Columns>
				requires (M1Columns == M2Rows)
			friend Matrix<M1Rows, M2Columns> (::operator*)(const Matrix<M1Rows, M1Columns>& lhs, const Matrix<M2Rows, M2Columns>& rhs);

		public:
			/// <summary>
			/// Constructs the identity matrix. This function is only valid in the case
			/// where the matrix is square (i.e., the number of rows is equal to the
			/// number of columns).
			/// </summary>
			template <typename Identity = void>
				requires (NumRows == NumColumns)
			Matrix();

			Matrix(typename const MatInfo::DirectXMatrixType& rhs);
			Matrix& operator=(typename const MatInfo::DirectXMatrixType& rhs);

			Matrix(typename MatInfo::DirectXMatrixType&& rhs) noexcept;
			Matrix& operator=(typename MatInfo::DirectXMatrixType&& rhs) noexcept;

			/// <summary>
			/// Constructs a matrix from an aiMatrix type.
			/// 
			/// NOTE: Assimp creates its matrices for *column* vectors. This constructor
			/// will transpose the provided matrix for use with *row* vectors. (That is
			/// why it is marked as explicit, as well as why there is no copy assignment
			/// operator.)
			/// </summary>
			/// <param name="rhs">
			/// - The Assimp matrix whose *TRANSPOSE* becomes the value of this
			///   Brawler::Math::Matrix instance.
			/// </param>
			explicit Matrix(typename const MatInfo::AssimpMatrixType& rhs);

			/// <summary>
			/// Attempts to compute the inverse of this matrix. If the inverse is undefined
			/// (i.e., its determinant is zero), then the inverse is undefined.
			/// </summary>
			/// <returns>
			/// If the determinant is a non-zero value, then the returned std::optional instance
			/// has a valid value, and this value is the inverse of this matrix; otherwise,
			/// the returned std::optional instance is empty.
			/// </returns>
			std::optional<Matrix> Inverse() const;
			
			Matrix<NumColumns, NumRows> Transpose() const;

			/// <summary>
			/// Computes the determinant of this matrix.
			/// </summary>
			/// <returns>
			/// The function returns the determinant of this matrix.
			/// </returns>
			float Determinant() const;

		private:
			/// <summary>
			/// Creates the equivalent DirectX::XMMATRIX for this matrix.
			/// </summary>
			/// <returns>
			/// The function returns the equivalent DirectX::XMMATRIX for this matrix.
			/// </returns>
			__forceinline DirectX::XMMATRIX XM_CALLCONV LoadMatrix() const;

		private:
			MatInfo::DirectXMatrixType mMatrix;
		};

		// -----------------------------------------------------------------------------------

		template <std::size_t NumRows, std::size_t NumColumns>
		template <typename Identity>
			requires (NumRows == NumColumns)
		Matrix<NumRows, NumColumns>::Matrix() :
			mMatrix()
		{
			MatInfo::StoreFunction(&mMatrix, DirectX::XMMatrixIdentity());
		};

		template <std::size_t NumRows, std::size_t NumColumns>
		Matrix<NumRows, NumColumns>::Matrix(typename const Matrix<NumRows, NumColumns>::MatInfo::DirectXMatrixType& rhs) :
			mMatrix(rhs)
		{}

		template <std::size_t NumRows, std::size_t NumColumns>
		Matrix<NumRows, NumColumns>& Matrix<NumRows, NumColumns>::operator=(typename const Matrix<NumRows, NumColumns>::MatInfo::DirectXMatrixType& rhs)
		{
			mMatrix = rhs;

			return *this;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		Matrix<NumRows, NumColumns>::Matrix(typename Matrix<NumRows, NumColumns>::MatInfo::DirectXMatrixType&& rhs) noexcept :
			mMatrix(std::move(rhs))
		{}

		template <std::size_t NumRows, std::size_t NumColumns>
		Matrix<NumRows, NumColumns>& Matrix<NumRows, NumColumns>::operator=(typename Matrix<NumRows, NumColumns>::MatInfo::DirectXMatrixType&& rhs) noexcept
		{
			mMatrix = std::move(rhs);

			return *this;
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		Matrix<NumRows, NumColumns>::Matrix(typename const Matrix<NumRows, NumColumns>::MatInfo::AssimpMatrixType& rhs) :
			mMatrix()
		{
			// Assimp stores its matrices in row-order format, just like DirectXMath. However, the
			// matrices are created for *column* vectors, whereas we use row vectors. So, when
			// dealing with Assimp's matrices, we take their transpose and store that, instead.

			typename MatInfo::AssimpMatrixType transposedRhsMatrix{ rhs };
			transposedRhsMatrix.Transpose();

			static_assert(sizeof(mMatrix) == sizeof(rhs), "ERROR: There is a size inconsistency between the DirectX matrix type and the Assimp matrix type of a Brawler::Math::Matrix! This *WILL* lead to errors converting between the two!");
			std::memcpy(&mMatrix, &transposedRhsMatrix, sizeof(transposedRhsMatrix));
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		std::optional<Matrix<NumRows, NumColumns>> Matrix<NumRows, NumColumns>::Inverse() const
		{
			const DirectX::XMMATRIX inverseMatrix{ DirectX::XMMatrixInverse(nullptr, LoadMatrix()) };

			// If DirectXMath returns an infinite matrix, then the inverse is undefined.
			if (DirectX::XMMatrixIsInfinite(inverseMatrix)) [[unlikely]]
				return std::optional<Matrix<NumRows, NumColumns>>{};

			typename MatInfo::DirectXMatrixType inverseStorageMatrix{};
			MatInfo::StoreFunction(&inverseStorageMatrix, inverseMatrix);

			return std::optional<Matrix<NumRows, NumColumns>>{ std::move(inverseStorageMatrix) };
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		Matrix<NumColumns, NumRows> Matrix<NumRows, NumColumns>::Transpose() const
		{
			const DirectX::XMMATRIX transposedMatrix{ DirectX::XMMatrixTranspose(LoadMatrix()) };
			
			typename Matrix<NumColumns, NumRows>::MatInfo::DirectXMatrixType transposedStorageMatrix{};
			Matrix<NumColumns, NumRows>::MatInfo::StoreFunction(&transposedStorageMatrix, transposedMatrix);

			return Matrix<NumColumns, NumRows>{ std::move(transposedStorageMatrix) };
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		float Matrix<NumRows, NumColumns>::Determinant() const
		{
			const DirectX::XMVECTOR determinantVector{ DirectX::XMMatrixDeterminant(LoadMatrix()) };

			return DirectX::XMVectorGetX(determinantVector);
		}

		template <std::size_t NumRows, std::size_t NumColumns>
		__forceinline DirectX::XMMATRIX XM_CALLCONV Matrix<NumRows, NumColumns>::LoadMatrix() const
		{
			return MatInfo::LoadFunction(&mMatrix);
		}
	}
}

export
{
	template <std::size_t M1Rows, std::size_t M1Columns, std::size_t M2Rows, std::size_t M2Columns>
		requires (M1Columns == M2Rows)
	Brawler::Math::Matrix<M1Rows, M2Columns> operator*(const Brawler::Math::Matrix<M1Rows, M1Columns>& lhs, const Brawler::Math::Matrix<M2Rows, M2Columns>& rhs)
	{
		const DirectX::XMMATRIX fastLHSMatrix{ lhs.LoadMatrix() };
		const DirectX::XMMATRIX fastRHSMatrix{ rhs.LoadMatrix() };

		const DirectX::XMMATRIX lhsTimesRhsMatrix{ DirectX::XMMatrixMultiply(fastLHSMatrix, fastRHSMatrix) };

		typename Brawler::Math::Matrix<M1Rows, M2Columns>::MatInfo::DirectXMatrixType storageMatrix{};
		Brawler::Math::Matrix<M1Rows, M2Columns>::MatInfo::StoreFunction(&storageMatrix, lhsTimesRhsMatrix);

		return Brawler::Math::Matrix<M1Rows, M2Columns>{ std::move(storageMatrix) };
	}
}

export namespace Brawler
{
	namespace Math
	{
		using Matrix4x4 = Matrix<4, 4>;
	}
}
module;
#include <span>
#include <cassert>
#include "DxDef.h"

export module Brawler.D3D12.TextureCopyBufferSubAllocation;
import Brawler.D3D12.I_BufferSubAllocation;
import Util.Math;

export namespace Brawler
{
	namespace D3D12
	{
		class TextureSubResource;
	}
}

export namespace Brawler
{
	namespace D3D12
	{
		class TextureCopyBufferSubAllocation final : public I_BufferSubAllocation
		{
		private:
			struct TextureInfo
			{
				D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint;
				std::uint32_t RowCount;
				std::size_t UnpaddedRowSizeInBytes;
				std::size_t TotalBytesRequired;
			};

		public:
			TextureCopyBufferSubAllocation() = default;
			explicit TextureCopyBufferSubAllocation(const TextureSubResource& textureSubResource);

			TextureCopyBufferSubAllocation(const TextureCopyBufferSubAllocation& rhs) = delete;
			TextureCopyBufferSubAllocation& operator=(const TextureCopyBufferSubAllocation& rhs) = delete;

			TextureCopyBufferSubAllocation(TextureCopyBufferSubAllocation&& rhs) noexcept = default;
			TextureCopyBufferSubAllocation& operator=(TextureCopyBufferSubAllocation&& rhs) noexcept = default;

			std::size_t GetSubAllocationSize() const override;
			std::size_t GetRequiredDataPlacementAlignment() const override;

			void OnReservationAssigned() override;

			template <typename T>
			void WriteTextureData(const std::uint32_t rowIndex, const std::span<const T> srcDataSpan) const;

			template <typename T>
			void ReadTextureData(const std::uint32_t rowIndex, const std::span<T> destDataSpan) const;

			std::uint32_t GetRowCount() const;

			/// <summary>
			/// Returns the size, in bytes, of a row of texture data for this TextureCopyBufferSubAllocation
			/// instance. Calling code should use this function to determine how large their std::span instances must
			/// be for writing and reading texture data.
			/// 
			/// *NOTE*: Internally, the D3D12 API requires rows of texture data within buffers to be aligned to
			/// D3D12_TEXTURE_DATA_PITCH_ALIGNMENT. This is handled automatically by the Brawler Engine. When creating
			/// std::span instances for WriteTextureData() and ReadTextureData(), use the value returned by this
			/// function without modifying or aligning it to determine how large these std::span instances must be.
			/// </summary>
			/// <returns>
			/// The function returns the size, in bytes, of a row of texture data for this TextureCopyBufferSubAllocation
			/// instance.
			/// </returns>
			std::size_t GetRowSizeInBytes() const;

			CD3DX12_TEXTURE_COPY_LOCATION GetBufferTextureCopyLocation() const;

		private:
			void InitializeTextureInfo(const TextureSubResource& textureSubResource);

		private:
			TextureInfo mTextureInfo;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace D3D12
	{
		template <typename T>
		void TextureCopyBufferSubAllocation::WriteTextureData(const std::uint32_t rowIndex, const std::span<const T> srcDataSpan) const
		{
			assert(srcDataSpan.size_bytes() <= mTextureInfo.UnpaddedRowSizeInBytes && "ERROR: An excess amount of data was provided for copying one row of texture data into a TextureCopyBufferSubAllocation via a call to TextureCopyBufferSubAllocation::WriteTextureData()!");
			assert(rowIndex < mTextureInfo.RowCount && "ERROR: An out-of-bounds row index was provided in a call to TextureCopyBufferSubAllocation::WriteTextureData()!");

			// Texture data in buffers is stored in row-major format, but the row sizes are aligned to
			// D3D12_TEXTURE_DATA_PITCH_ALIGNMENT.
			const std::size_t offsetFromSubAllocationStart = (static_cast<std::size_t>(rowIndex) * Util::Math::AlignToPowerOfTwo(mTextureInfo.UnpaddedRowSizeInBytes, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
			WriteToBuffer(srcDataSpan, offsetFromSubAllocationStart);
		}

		template <typename T>
		void TextureCopyBufferSubAllocation::ReadTextureData(const std::uint32_t rowIndex, const std::span<T> destDataSpan) const
		{
			assert(destDataSpan.size_bytes() == mTextureInfo.UnpaddedRowSizeInBytes && "ERROR: An attempt was made to read a row of texture data in a call to TextureCopyBufferSubAllocation::ReadTextureData(), but the provided std::span for storing the read data was not equal in size to the unpadded size of a row of texture data!");
			assert(rowIndex <= mTextureInfo.RowCount && "ERROR: An out-of-bounds row index was provided in a call to TextureCopyBufferSubAllocation::ReadTextureData()!");

			// Texture data in buffers is stored in row-major format, but the row sizes are aligned to
			// D3D12_TEXTURE_DATA_PITCH_ALIGNMENT.
			const std::size_t offsetFromSubAllocationStart = (static_cast<std::size_t>(rowIndex) * Util::Math::AlignToPowerOfTwo(mTextureInfo.UnpaddedRowSizeInBytes, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
			ReadFromBuffer(destDataSpan, offsetFromSubAllocationStart);
		}
	}
}
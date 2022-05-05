module;
#include <span>
#include <vector>
#include <zstd.h>

export module Brawler.I_ZSTDContext;

export namespace Brawler
{
	class I_ZSTDContext
	{
	protected:
		I_ZSTDContext();

	public:
		virtual ~I_ZSTDContext();

		I_ZSTDContext(const I_ZSTDContext& rhs) = delete;
		I_ZSTDContext& operator=(const I_ZSTDContext& rhs) = delete;

		I_ZSTDContext(I_ZSTDContext&& rhs) noexcept;
		I_ZSTDContext& operator=(I_ZSTDContext&& rhs) noexcept;

		/// <summary>
		/// Begins consuming input on the current input stream. The function will attempt
		/// to decompress outputSizeInBytes bytes from the currently set input stream, but
		/// it may or may not actually return this many bytes.
		/// </summary>
		/// <param name="numBlocks">
		/// - The number of ZSTD blocks to decompress. One can query the total size in bytes
		///   of this many blocks by calling I_ZSTDContext::GetBlockSize().
		/// </param>
		/// <returns>
		/// The function returns a std::vector<std::uint8_t> containing the bytes created
		/// by decompressing parts of the input stream. The actual size of this std::vector
		/// may differ from the size specified by outputSizeInBytes.
		/// </returns>
		std::vector<std::uint8_t> DecompressData(const std::size_t numBlocks = 1);

		/// <summary>
		/// Retrieves the size, in bytes, of an arbitrary amount of ZSTD blocks after
		/// decompression.
		/// </summary>
		/// <param name="numBlocks">
		/// - The number of blocks to get the size for. By default, the size of one
		///   block is returned.
		/// </param>
		/// <returns>
		/// This function returns the size, in bytes, of an arbitrary amount of ZSTD
		/// blocks after decompression.
		/// </returns>
		std::size_t GetBlockSize(const std::size_t numBlocks = 1) const;

	protected:
		/// <summary>
		/// Derived classes should override this to return a std::span<std::uint8_t> containing
		/// numBytesToFetch bytes for decompression. This function is called in
		/// I_ZSTDContext::DecompressData() as needed to retrieve the input for the next block.
		/// </summary>
		/// <param name="numBytesToFetch">
		/// - The size, in bytes, of input data required for the next block for decompression.
		/// </param>
		/// <returns>
		/// Derived classes should implement this function to return a std::span<std::uint8_t>
		/// which refers to the bytes containing the input for the next ZSTD block for
		/// decompression.
		/// </returns>
		virtual std::span<const std::uint8_t> FetchInputData(const std::size_t numBytesToFetch) = 0;

		/// <summary>
		/// Sometimes, it is not acceptable to merely finish decompression after processing
		/// all input. For example, background music might loop indefinitely, so streaming
		/// for that should not end until the next song is to be played.
		/// 
		/// Derived classes should implement this function to let the base class know that
		/// there are no further ZSTD blocks which need to be decompressed; that way, it
		/// will not call I_ZSTDContext::FetchInputData() again.
		/// </summary>
		/// <returns>
		/// Derived classes should implement this function to return true if there are
		/// additional ZSTD blocks which need to be decompressed and false otherwise.
		/// Read the summary for additional insight as to why decompression may never end.
		/// </returns>
		virtual bool IsInputFinished() const = 0;

		void ResetDecompressionContext();

	private:
		void DeleteDecompressionContext();

	private:
		ZSTD_DCtx* mContextPtr;
	};
}
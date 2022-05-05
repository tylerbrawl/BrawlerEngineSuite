module;
#include <span>
#include <string>

export module Util.FileWrite;

export namespace Util
{
	namespace FileWrite
	{
		/// <summary>
		/// Begins the serialization process for writing out all of the source files for the current
		/// shader profile.
		/// </summary>
		void SerializeSourceFiles();

		/// <summary>
		/// Given a std::span of bytes, this function returns a string whose contents can
		/// be written to a source file as the initializer list parameters of a std::array<std::uint8_t, X>.
		/// </summary>
		/// <param name="byteSpan">
		/// - The span of bytes which are to be used to create a string from.
		/// </param>
		/// <returns>
		/// The function returns a string whose contents can be written to a source file as the initializer 
		/// list parameters of a std::array<std::uint8_t, X>.
		/// </returns>
		std::string CreateSTDArrayContentsStringFromBuffer(std::span<const std::uint8_t> byteSpan);
	}
}
module;
#include <span>
#include <string>
#include <vector>
#include <ranges>
#include <memory> // Classic MSVC modules jank...

module Util.FileWrite;
import Brawler.AppParams;
import Util.General;
import Brawler.RootSignatureDefinitionsFileWriter;
import Brawler.RootSignatureIDsFileWriter;
import Brawler.PSOIDsFileWriter;
import Brawler.PSODefinitionsFileWriter;
import Brawler.RootParameterEnumsFileWriter;
import Brawler.ShaderProfileID;
import Brawler.JobSystem;

namespace
{
	void AddCharactersToCharacterArrayForByte(std::vector<std::uint8_t>& charArr, const std::uint8_t byte)
	{
		static constexpr std::string_view HEX_STR{ "0123456789ABCDEF" };
		
		charArr.push_back('0');
		charArr.push_back('x');

		const std::uint8_t upperFourBits = ((byte & 0xF0) >> 4);
		charArr.push_back(HEX_STR[upperFourBits]);

		const std::uint8_t lowerFourBits = (byte & 0xF);
		charArr.push_back(HEX_STR[lowerFourBits]);
	}

	template <Brawler::ShaderProfiles::ShaderProfileID CurrProfileID>
	void AddSerializationJobsForShaderProfile(Brawler::JobGroup& serializationJobGroup)
	{
		if (CurrProfileID == Util::General::GetLaunchParameters().ShaderProfile)
		{
			serializationJobGroup.AddJob([] ()
			{
				Brawler::SourceFileWriters::RootSignatureDefinitionsFileWriter<CurrProfileID> rootSignaturesWriter{};
				rootSignaturesWriter.WriteSourceFile();
			});

			serializationJobGroup.AddJob([] ()
			{
				Brawler::SourceFileWriters::RootSignatureIDsFileWriter<CurrProfileID> rootSignaturesIDWriter{};
				rootSignaturesIDWriter.WriteSourceFile();
			});

			serializationJobGroup.AddJob([] ()
			{
				Brawler::SourceFileWriters::RootParameterEnumsFileWriter<CurrProfileID> rootParametersWriter{};
				rootParametersWriter.WriteSourceFile();
			});

			serializationJobGroup.AddJob([] ()
			{
				Brawler::SourceFileWriters::PSOIDsFileWriter<CurrProfileID> pSOsIDWriter{};
				pSOsIDWriter.WriteSourceFile();
			});

			serializationJobGroup.AddJob([] ()
			{
				Brawler::SourceFileWriters::PSODefinitionsFileWriter<CurrProfileID> pSODefinitionsWriter{};
				pSODefinitionsWriter.WriteSourceFile();
			});

			return;
		}

		constexpr Brawler::ShaderProfiles::ShaderProfileID NEXT_PROFILE_ID = static_cast<Brawler::ShaderProfiles::ShaderProfileID>(std::to_underlying(CurrProfileID) + 1);
		if constexpr (NEXT_PROFILE_ID != Brawler::ShaderProfiles::ShaderProfileID::COUNT_OR_ERROR)
			AddSerializationJobsForShaderProfile<NEXT_PROFILE_ID>(serializationJobGroup);
	}
}

namespace Util
{
	namespace FileWrite
	{
		void SerializeSourceFiles()
		{
			Brawler::JobGroup serializationJobGroup{};
			serializationJobGroup.Reserve(5);

			AddSerializationJobsForShaderProfile<static_cast<Brawler::ShaderProfiles::ShaderProfileID>(0)>(serializationJobGroup);

			serializationJobGroup.ExecuteJobs();
		}
		
		std::string CreateSTDArrayContentsStringFromBuffer(std::span<const std::uint8_t> byteSpan)
		{
			if (byteSpan.empty()) [[unlikely]]
				return std::string{};
			
			// We can expect a lot of heap allocations to occur if we append one byte at a time to the
			// std::string. So, what we do instead is create a std::vector<std::uint8_t>, write the
			// characters into that, and then copy the contents of this buffer into a std::string instance.
			std::vector<std::uint8_t> byteCharArr{};
			
			// Every byte is 4 characters (0x??). We need a comma after all except the last byte in the
			// span. After that, we need to manually add a null-terminating character. So, our final
			// std::vector<std::uint8_t> size is (4 * byteSpan.size()) + (byteSpan.size() - 1) + 1 ==
			// (4 * byteSpan.size()) + byteSpan.size() == (5 * byteSpan.size()).
			byteCharArr.reserve(5 * byteSpan.size());

			// Add our first byte to the character array.
			AddCharactersToCharacterArrayForByte(byteCharArr, byteSpan[0]);

			// Now, add a comma, followed by the next byte. Do this for all of the remaining bytes in
			// the span.
			for (const auto byte : byteSpan | std::views::drop(1))
			{
				byteCharArr.push_back(',');
				AddCharactersToCharacterArrayForByte(byteCharArr, byte);
			}

			// Finally, add the null-terminating character.
			byteCharArr.push_back(0);

			// Return a std::string constructed from these characters.
			return std::string{ reinterpret_cast<char*>(byteCharArr.data()) };
		}
	}
}
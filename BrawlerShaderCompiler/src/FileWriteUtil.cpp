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
import Brawler.PSODefinitionSpecializationFileWriter;
import Brawler.PSODefinitionBaseFileWriter;
import Brawler.CommandSignatureIDsFileWriter;
import Brawler.CommandSignatureDefinitionSpecializationFileWriter;
import Brawler.CommandSignatureDefinitionBaseFileWriter;
import Brawler.CommandSignatureDefinitionsFileWriter;
import Brawler.ShaderProfileID;
import Brawler.ShaderProfileDefinition;
import Brawler.JobSystem;
import Brawler.PSOID;
import Brawler.CommandSignatureID;

namespace
{
	static constexpr std::string_view HEX_STR{ "0123456789ABCDEF" };
	
	void AddCharactersToCharacterArrayForByte(std::vector<std::uint8_t>& charArr, const std::uint8_t byte)
	{
		charArr.push_back('0');
		charArr.push_back('x');

		const std::uint8_t upperFourBits = ((byte & 0xF0) >> 4);
		charArr.push_back(HEX_STR[upperFourBits]);

		const std::uint8_t lowerFourBits = (byte & 0xF);
		charArr.push_back(HEX_STR[lowerFourBits]);
	}

	void AddCharactersToCharacterArrayFor8Bytes(std::vector<std::uint8_t>& charArr, const std::span<const std::uint8_t> byteSpan)
	{
		charArr.push_back('0');
		charArr.push_back('x');

		// What follows might hurt your brain a little. We need to reverse the order
		// in which the bytes are written out into a string due to endianness.
		//
		// The naive method would write out the bytes in the order in which they appear
		// in memory. This works fine when writing out binary values, but it does NOT work
		// when writing out bytes into a string representing an integer larger than a byte.
		//
		// As an example, consider the 32-bit integer 0x12345678. If we write out the bytes
		// directly in the order they appear in memory into a source file, then what we would 
		// get is as we expect: the hexadecimal literal 0x12345678. This is fine if our intention 
		// really was to write out this value, but our goal is to write out a set of bytes.
		//
		// For our case, the problem comes when using it in the initializer list of a
		// std::array<std::uint32_t> which is being used to represent an array of bytes, e.g.,
		// for shader bytecode. The compiler will see that we have written out the literal value
		// 0x12345678, but since we tell it that it is an array of 32-bit integers, it swaps the 
		// order of bytes to 0x78563412 when it writes the value out to static data. It does this 
		// because Windows is a little-endian system, and the compiler is assuming that we are 
		// going to be using the array as an array of 32-bit integers (and rightfully so).
		//
		// However, because the compiler wrote the bytes out as static data in reverse order, if
		// we then attempt to reinterpret_cast the data pointer of the std::array<std::uint32_t>
		// to an array of bytes, we will find that each 4 bytes have been reversed from the order
		// in which they were written out in the source file!
		//
		// The solution, then, is to write out the bytes in the *reverse* order in which they
		// appear in memory. That way, when the compiler swaps the bytes around to account for
		// endianness, they will be written to static data in the order in which we intended.

		// ---------------------------------------------------------------------------------------------

		// If byteSpan did not have enough bytes to fill a 64-bit integer, then
		// fill the remaining bits with zeroes.
		{
			std::size_t numZeroBytesToAdd = std::max<std::size_t>((8 - byteSpan.size()), 0);

			while (numZeroBytesToAdd > 0)
			{
				charArr.push_back('0');
				charArr.push_back('0');

				--numZeroBytesToAdd;
			}
		}
		

		for (const auto byte : byteSpan | std::views::take(8) | std::views::reverse)
		{
			charArr.push_back(HEX_STR[(byte & 0xF0) >> 4]);
			charArr.push_back(HEX_STR[byte & 0xF]);
		}
	}

	template <Brawler::ShaderProfiles::ShaderProfileID CurrProfileID>
	void AddSerializationJobsForShaderProfile(Brawler::JobGroup& serializationJobGroup)
	{
		if (CurrProfileID == Util::General::GetLaunchParameters().ShaderProfile)
		{
			constexpr auto RELEVANT_PSO_ID_LIST{ Brawler::ShaderProfiles::GetPSOIdentifiers<CurrProfileID>() };
			constexpr auto RELEVANT_COMMAND_SIGNATURE_ID_ARR{ Brawler::ShaderProfiles::GetCommandSignatureIdentifiers<CurrProfileID>() };

			constexpr std::size_t RESERVED_JOB_GROUP_ALLOCATION_SIZE = 9 + RELEVANT_PSO_ID_LIST.size() + RELEVANT_COMMAND_SIGNATURE_ID_ARR.size();
			serializationJobGroup.Reserve(RESERVED_JOB_GROUP_ALLOCATION_SIZE);
			
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

			serializationJobGroup.AddJob([] ()
			{
				Brawler::SourceFileWriters::PSODefinitionsBaseFileWriter basePSODefinitionWriter{};
				basePSODefinitionWriter.WriteSourceFile();
			});

			serializationJobGroup.AddJob([] ()
			{
				Brawler::SourceFileWriters::CommandSignatureIDsFileWriter<CurrProfileID> cmdSignaturesIDWriter{};
				cmdSignaturesIDWriter.WriteSourceFile();
			});

			serializationJobGroup.AddJob([] ()
			{
				Brawler::SourceFileWriters::CommandSignatureDefinitionBaseFileWriter baseCMDSignatureWriter{};
				baseCMDSignatureWriter.WriteSourceFile();
			});

			serializationJobGroup.AddJob([] ()
			{
				Brawler::SourceFileWriters::CommandSignatureDefinitionsFileWriter<CurrProfileID> cmdSignatureDefinitionsWriter{};
				cmdSignatureDefinitionsWriter.WriteSourceFile();
			});

			constexpr auto ADD_PSO_DEFINITION_SPECIALIZATION_JOBS_LAMBDA = []<std::underlying_type_t<Brawler::PSOID>... PSOIdentifierNums>(Brawler::JobGroup& jobGroup, std::integer_sequence<std::underlying_type_t<Brawler::PSOID>, PSOIdentifierNums...> psoSequence)
			{
				constexpr auto ADD_SINGLE_JOB_LAMBDA = []<Brawler::PSOID PSOIdentifier>(Brawler::JobGroup& jobGroup)
				{
					jobGroup.AddJob([] ()
					{
						Brawler::SourceFileWriters::PSODefinitionSpecializationFileWriter<PSOIdentifier> psoDefinitionWriter{};
						psoDefinitionWriter.WriteSourceFile();
					});
				};

				((ADD_SINGLE_JOB_LAMBDA.operator()<static_cast<Brawler::PSOID>(PSOIdentifierNums)>(jobGroup)), ...);
			};
			ADD_PSO_DEFINITION_SPECIALIZATION_JOBS_LAMBDA(serializationJobGroup, RELEVANT_PSO_ID_LIST);

			constexpr auto ADD_COMMAND_SIGNATURE_DEFINITION_SPECIALIZATION_JOBS_LAMBDA = []<std::size_t CurrIndex>(this const auto& self, Brawler::JobGroup& jobGroup)
			{
				if constexpr (CurrIndex != RELEVANT_COMMAND_SIGNATURE_ID_ARR.size())
				{
					constexpr Brawler::CommandSignatureID CURR_IDENTIFIER = RELEVANT_COMMAND_SIGNATURE_ID_ARR[CurrIndex];

					jobGroup.AddJob([] ()
					{
						Brawler::SourceFileWriters::CommandSignatureDefinitionSpecializationFileWriter<CURR_IDENTIFIER> cmdSignatureDefinitionWriter{};
						cmdSignatureDefinitionWriter.WriteSourceFile();
					});

					constexpr std::size_t NEXT_INDEX = (CurrIndex + 1);
					self.template operator()<NEXT_INDEX>(jobGroup);
				}
			};
			ADD_COMMAND_SIGNATURE_DEFINITION_SPECIALIZATION_JOBS_LAMBDA.template operator()<0>(serializationJobGroup);

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
			AddSerializationJobsForShaderProfile<static_cast<Brawler::ShaderProfiles::ShaderProfileID>(0)>(serializationJobGroup);

			serializationJobGroup.ExecuteJobs();
		}
		
		std::string CreateSTDUInt8ArrayContentsStringFromBuffer(std::span<const std::uint8_t> byteSpan)
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

		Brawler::CondensedByteArrayInfo CreateSTDUInt64ArrayContentsStringFromBuffer(std::span<const std::uint8_t> byteSpan)
		{
			if (byteSpan.empty()) [[unlikely]]
				return Brawler::CondensedByteArrayInfo{};

			// Much like with the above function, we will allocate a std::vector<std::uint8_t> to hold all of
			// the required characters.
			std::vector<std::uint8_t> byteCharArr{};

			const std::size_t numBytes = byteSpan.size_bytes();
			const std::size_t num64BitIntegers = (numBytes / 8 + (numBytes % 8 == 0 ? 0 : 1));

			// Each 64-bit integer is represented as a hexadecimal value with 18 characters
			// (0x????????????????). We need a comma after every integer except for the last one,
			// as well as a null-terminating character at the end. So, we need a total of
			// (18 * num64BitIntegers) + (num64BitIntegers - 1) + 1 ==
			// (18 * num64BitIntegers) + num64BitIntegers == (19 * num64BitIntegers) characters
			// in our array.
			byteCharArr.reserve(19 * num64BitIntegers);

			AddCharactersToCharacterArrayFor8Bytes(byteCharArr, byteSpan.subspan(0, std::min<std::size_t>(byteSpan.size(), 8)));
			byteSpan = byteSpan.subspan(std::min<std::size_t>(byteSpan.size(), 8));

			while (!byteSpan.empty())
			{
				byteCharArr.push_back(',');

				AddCharactersToCharacterArrayFor8Bytes(byteCharArr, byteSpan.subspan(0, std::min<std::size_t>(byteSpan.size(), 8)));
				byteSpan = byteSpan.subspan(std::min<std::size_t>(byteSpan.size(), 8));
			}

			// Add the null-terminating character.
			byteCharArr.push_back(0);

			return Brawler::CondensedByteArrayInfo{
				.UInt64ByteArrayInitializerListContents{ reinterpret_cast<char*>(byteCharArr.data()) },
				.InitializerListLengthInElements = num64BitIntegers,
				.ActualDataSizeInBytes = numBytes
			};
		}
	}
}
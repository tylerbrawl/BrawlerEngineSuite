// WARNING: This file was auto-generated by the Brawler Shader Compiler. You will incur the
// wrath of God if you dare touch it.

module;
#include <array>
#include <span>

export module Brawler.RootSignatures.RootSignatureDefinition;
import Brawler.RootSignatures.RootSignatureID;
import Brawler.RootParameters.RootParameterEnums;

namespace Brawler
{
	namespace RootSignatures
	{
		template <RootSignatureID RSIdentifier>
		struct RootSignatureDefinition
		{};

		template <>
		struct RootSignatureDefinition<RootSignatureID::TEST_ROOT_SIGNATURE>
		{
			static constexpr std::array<std::uint8_t, 384> SERIALIZED_ROOT_SIGNATURE_VERSION_1_0{0x44,0x58,0x42,0x43,0x57,0xDD,0xFB,0x6E,0xC9,0x6C,0xD8,0x86,0x3A,0xDA,0x2D,0x0E,0x7D,0xB0,0x9F,0x28,0x01,0x00,0x00,0x00,0x80,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x52,0x54,0x53,0x30,0x54,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x20,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xA8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xB0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xB8,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0xD4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x0C,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x55,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x7F,0x7F,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
			static constexpr std::array<std::uint8_t, 412> SERIALIZED_ROOT_SIGNATURE_VERSION_1_1{0x44,0x58,0x42,0x43,0x68,0xD7,0x12,0x05,0x91,0x37,0x85,0x2C,0x1D,0x66,0x0B,0x2C,0x0E,0xCB,0x35,0x75,0x01,0x00,0x00,0x00,0x9C,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x52,0x54,0x53,0x30,0x70,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x3C,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xB0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xBC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xDC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xC4,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0xE4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x04,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x24,0x01,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x55,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x7F,0x7F,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

			using RootParamEnumType = Brawler::RootParameters::TestRootSignature;

			static constexpr std::array<Brawler::RootParameters::RootParameterType, std::to_underlying(RootParamEnumType::COUNT_OR_ERROR)> ROOT_PARAM_TYPES_ARR{Brawler::RootParameters::RootParameterType::ROOT_CONSTANT,Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE,Brawler::RootParameters::RootParameterType::CBV,Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE,Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE,Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE,Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE};
		};

	}
}

export namespace Brawler
{
	namespace RootSignatures
	{
		template <RootSignatureID RSIdentifier>
		using RootParamEnumType = RootSignatureDefinition<RSIdentifier>::RootParamEnumType;

		template <RootSignatureID RSIdentifier>
		consteval auto GetSerializedRootSignature1_0()
		{
			return std::span<const std::uint8_t, RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_0.size()>{ RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_0 };
		}

		template <RootSignatureID RSIdentifier>
		consteval auto GetSerializedRootSignature1_1()
		{
			return std::span<const std::uint8_t, RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_1.size()>{ RootSignatureDefinition<RSIdentifier>::SERIALIZED_ROOT_SIGNATURE_VERSION_1_1 };
		}

		template <RootSignatureID RSIdentifier, RootParamEnumType<RSIdentifier> RootParam>
		consteval Brawler::RootParameters::RootParameterType GetRootParameterType()
		{
			return RootSignatureDefinition<RSIdentifier>::ROOT_PARAM_TYPES_ARR[std::to_underlying(RootParam)];
		}
	}
}
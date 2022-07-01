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
		struct RootSignatureDefinition<RootSignatureID::BC6H_BC7_COMPRESSION>
		{
			static constexpr std::array<std::uint8_t, 232> SERIALIZED_ROOT_SIGNATURE_VERSION_1_0{0x44,0x58,0x42,0x43,0xCB,0xEC,0x74,0xAE,0x66,0xA2,0xB9,0xCF,0xB3,0x52,0x6C,0x19,0x71,0x9F,0xB8,0xED,0x01,0x00,0x00,0x00,0xE8,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x52,0x54,0x53,0x30,0xBC,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xBC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x54,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x8C,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xA8,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xB0,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x5C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x94,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00};
			static constexpr std::array<std::uint8_t, 248> SERIALIZED_ROOT_SIGNATURE_VERSION_1_1{0x44,0x58,0x42,0x43,0x11,0xF4,0x52,0x07,0x40,0x2E,0x41,0xA7,0x3D,0x72,0xBE,0x4E,0xF0,0xBC,0xED,0xD4,0x01,0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x52,0x54,0x53,0x30,0xCC,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x54,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x74,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x94,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xB4,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x5C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x9C,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00};

			using RootParamEnumType = Brawler::RootParameters::BC6HBC7Compression;

			static constexpr std::array<Brawler::RootParameters::RootParameterType, std::to_underlying(RootParamEnumType::COUNT_OR_ERROR)> ROOT_PARAM_TYPES_ARR{Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE,Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE,Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE,Brawler::RootParameters::RootParameterType::CBV,Brawler::RootParameters::RootParameterType::ROOT_CONSTANT};
		};

		template <>
		struct RootSignatureDefinition<RootSignatureID::GENERIC_DOWNSAMPLE>
		{
			static constexpr std::array<std::uint8_t, 204> SERIALIZED_ROOT_SIGNATURE_VERSION_1_0{0x44,0x58,0x42,0x43,0x83,0x7C,0xDA,0x54,0x9C,0x12,0xA7,0x9A,0x98,0x1C,0xDC,0x16,0x89,0x37,0x0F,0x64,0x01,0x00,0x00,0x00,0xCC,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x52,0x54,0x53,0x30,0xA0,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x6C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x14,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x7F,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
			static constexpr std::array<std::uint8_t, 212> SERIALIZED_ROOT_SIGNATURE_VERSION_1_1{0x44,0x58,0x42,0x43,0x71,0x4F,0x93,0x39,0x9D,0xAA,0x40,0xBF,0xD6,0xC0,0x63,0xCB,0x13,0x10,0xF6,0x9C,0x01,0x00,0x00,0x00,0xD4,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x52,0x54,0x53,0x30,0xA8,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x74,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x68,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x14,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x7F,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

			using RootParamEnumType = Brawler::RootParameters::GenericDownsample;

			static constexpr std::array<Brawler::RootParameters::RootParameterType, std::to_underlying(RootParamEnumType::COUNT_OR_ERROR)> ROOT_PARAM_TYPES_ARR{Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE,Brawler::RootParameters::RootParameterType::ROOT_CONSTANT};
		};

		template <>
		struct RootSignatureDefinition<RootSignatureID::VIRTUAL_TEXTURE_PAGE_TILING>
		{
			static constexpr std::array<std::uint8_t, 244> SERIALIZED_ROOT_SIGNATURE_VERSION_1_0{0x44,0x58,0x42,0x43,0x16,0x4D,0x3F,0x66,0xDD,0xC9,0x8D,0xB9,0x65,0x80,0xAF,0x37,0xA6,0x58,0xFA,0x0E,0x01,0x00,0x00,0x00,0xF4,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x52,0x54,0x53,0x30,0xC8,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x94,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x54,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x5C,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x7F,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
			static constexpr std::array<std::uint8_t, 256> SERIALIZED_ROOT_SIGNATURE_VERSION_1_1{0x44,0x58,0x42,0x43,0x4D,0xC4,0xD3,0xA7,0x20,0x48,0xF8,0x0C,0x65,0x61,0x6A,0x03,0x85,0xFB,0x76,0xE8,0x01,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x52,0x54,0x53,0x30,0xD4,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x54,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x74,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x5C,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x7F,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

			using RootParamEnumType = Brawler::RootParameters::VirtualTexturePageTiling;

			static constexpr std::array<Brawler::RootParameters::RootParameterType, std::to_underlying(RootParamEnumType::COUNT_OR_ERROR)> ROOT_PARAM_TYPES_ARR{Brawler::RootParameters::RootParameterType::ROOT_CONSTANT,Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE,Brawler::RootParameters::RootParameterType::CBV,Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE};
		};

		template <>
		struct RootSignatureDefinition<RootSignatureID::VIRTUAL_TEXTURE_PAGE_MERGING>
		{
			static constexpr std::array<std::uint8_t, 224> SERIALIZED_ROOT_SIGNATURE_VERSION_1_0{0x44,0x58,0x42,0x43,0x62,0x66,0x6A,0xFF,0x89,0xCF,0x93,0xED,0x2D,0xF4,0x0A,0x58,0x83,0x3B,0xFA,0xED,0x01,0x00,0x00,0x00,0xE0,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x52,0x54,0x53,0x30,0xB4,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x50,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x6C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x7F,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
			static constexpr std::array<std::uint8_t, 232> SERIALIZED_ROOT_SIGNATURE_VERSION_1_1{0x44,0x58,0x42,0x43,0x99,0xB4,0x01,0xA1,0x56,0x12,0xC8,0xFD,0xD6,0x61,0xC2,0xF2,0xE9,0xED,0x9E,0xD2,0x01,0x00,0x00,0x00,0xE8,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x52,0x54,0x53,0x30,0xBC,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x68,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x50,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x7F,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

			using RootParamEnumType = Brawler::RootParameters::VirtualTexturePageMerging;

			static constexpr std::array<Brawler::RootParameters::RootParameterType, std::to_underlying(RootParamEnumType::COUNT_OR_ERROR)> ROOT_PARAM_TYPES_ARR{Brawler::RootParameters::RootParameterType::ROOT_CONSTANT,Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE,Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE};
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

		template <RootSignatureID RSIdentifier>
		consteval std::size_t GetRootParameterCount()
		{
			return static_cast<std::size_t>(RootParamEnumType<RSIdentifier>::COUNT_OR_ERROR);
		}
	}
}
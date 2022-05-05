module;

export module Brawler.RootSignatureID;

export namespace Brawler
{
	enum class RootSignatureID
	{
		// Brawler Model Exporter
		BC6H_BC7_COMPRESSION,

		// Test Shader Profile
		TEST_ROOT_SIGNATURE,
		
		COUNT_OR_ERROR
	};
}
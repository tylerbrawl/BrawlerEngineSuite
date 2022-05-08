module;
#include <array>
#include <memory>
#include <cassert>
#include <span>
#include "DxDef.h"

export module Brawler.RootSignatureBuilder;
import Brawler.RootSignatureID;
import Brawler.ConvenienceTypes;
import Brawler.RootSignatureDefinition;
import Brawler.RootParameterDefinition;
import Brawler.RootParameters;
import Brawler.FileWriterNode;
import Util.FileWrite;

namespace Brawler
{
	namespace RootSignatures
	{
		namespace IMPL
		{
			template <Brawler::RootParameters::RootParameterType ParamType>
			struct RootDescriptorInfo
			{
				static constexpr Brawler::RootParameters::RootParameterType ROOT_PARAM_TYPE = ParamType;

				std::uint32_t ShaderRegister;
				std::uint32_t RegisterSpace;
				D3D12_ROOT_DESCRIPTOR_FLAGS Flags;
				D3D12_SHADER_VISIBILITY Visibility;
			};
		}
	}
}

export namespace Brawler
{
	namespace RootSignatures
	{
		struct RootConstantsInfo
		{
			std::uint32_t Num32BitValues;
			std::uint32_t ShaderRegister;
			std::uint32_t RegisterSpace;
			D3D12_SHADER_VISIBILITY Visibility;
		};

		using RootCBVInfo = IMPL::RootDescriptorInfo <Brawler::RootParameters::RootParameterType::CBV>;
		using RootSRVInfo = IMPL::RootDescriptorInfo<Brawler::RootParameters::RootParameterType::SRV>;
		using RootUAVInfo = IMPL::RootDescriptorInfo<Brawler::RootParameters::RootParameterType::UAV>;

		struct DescriptorTableInfo
		{
			std::vector<CD3DX12_DESCRIPTOR_RANGE1> DescriptorRangeArr;
			D3D12_SHADER_VISIBILITY Visibility;
		};
	}
}

namespace Brawler
{
	namespace RootSignatures
	{
		namespace IMPL
		{
			using RootDescriptorInitFunction_T = Brawler::FunctionPtr<void, D3D12_ROOT_PARAMETER1&, UINT, UINT, D3D12_ROOT_DESCRIPTOR_FLAGS, D3D12_SHADER_VISIBILITY>;

			class I_RootParameter
			{
			protected:
				I_RootParameter() = default;

			public:
				virtual ~I_RootParameter() = default;

				virtual void InitializeRootParameter(CD3DX12_ROOT_PARAMETER1& rootParam) const = 0;
				virtual std::string_view GetRootParameterTypeString() const = 0;
				virtual std::uint32_t GetRootSignatureCost() const = 0;
			};

			class RootConstantsParameter final : public I_RootParameter
			{
			public:
				explicit RootConstantsParameter(Brawler::RootSignatures::RootConstantsInfo&& paramInfo);

				void InitializeRootParameter(CD3DX12_ROOT_PARAMETER1& rootParam) const override;
				std::string_view GetRootParameterTypeString() const override;
				std::uint32_t GetRootSignatureCost() const override;

			private:
				Brawler::RootSignatures::RootConstantsInfo mParamInfo;
			};

			template <typename ParamInfo>
			class RootDescriptorParameter final : public I_RootParameter
			{
			public:
				explicit RootDescriptorParameter(ParamInfo&& paramInfo);

				void InitializeRootParameter(CD3DX12_ROOT_PARAMETER1& rootParam) const override;
				std::string_view GetRootParameterTypeString() const override;
				std::uint32_t GetRootSignatureCost() const override;

			private:
				ParamInfo mParamInfo;
			};

			using RootCBVParameter = RootDescriptorParameter<Brawler::RootSignatures::RootCBVInfo>;
			using RootSRVParameter = RootDescriptorParameter<Brawler::RootSignatures::RootSRVInfo>;
			using RootUAVParameter = RootDescriptorParameter<Brawler::RootSignatures::RootUAVInfo>;

			class DescriptorTableParameter final : public I_RootParameter
			{
			public:
				explicit DescriptorTableParameter(Brawler::RootSignatures::DescriptorTableInfo&& paramInfo);

				void InitializeRootParameter(CD3DX12_ROOT_PARAMETER1& rootParam) const override;
				std::string_view GetRootParameterTypeString() const override;
				std::uint32_t GetRootSignatureCost() const override;

			private:
				Brawler::RootSignatures::DescriptorTableInfo mParamInfo;
			};

			template <typename T>
			struct RootParameterInfoMap
			{
				static_assert(sizeof(T) != sizeof(T));
			};

			template <typename RootParamType_, RootDescriptorInitFunction_T InitFunction = nullptr>
			struct RootParameterInfoMapInstantiation
			{
				using RootParamType = RootParamType_;
				static constexpr RootDescriptorInitFunction_T INITIALIZATION_FUNCTION = InitFunction;
			};

			template <>
			struct RootParameterInfoMap<Brawler::RootSignatures::RootConstantsInfo> : public RootParameterInfoMapInstantiation<RootConstantsParameter>
			{};

			// There's something fishy going on with the way the MSVC is handling static function pointers across modules,
			// so we create our own redundant non-static functions and use function pointers to that.

			void InitializeCBV(
				D3D12_ROOT_PARAMETER1& rootParam,
				UINT shaderRegister,
				UINT registerSpace,
				D3D12_ROOT_DESCRIPTOR_FLAGS flags,
				D3D12_SHADER_VISIBILITY visibility
			)
			{
				CD3DX12_ROOT_PARAMETER1::InitAsConstantBufferView(
					rootParam,
					shaderRegister,
					registerSpace,
					flags,
					visibility
				);
			}

			template <>
			struct RootParameterInfoMap<Brawler::RootSignatures::RootCBVInfo> : public RootParameterInfoMapInstantiation<RootCBVParameter, InitializeCBV>
			{};

			void InitializeSRV(
				D3D12_ROOT_PARAMETER1& rootParam,
				UINT shaderRegister,
				UINT registerSpace,
				D3D12_ROOT_DESCRIPTOR_FLAGS flags,
				D3D12_SHADER_VISIBILITY visibility
			)
			{
				CD3DX12_ROOT_PARAMETER1::InitAsShaderResourceView(
					rootParam,
					shaderRegister,
					registerSpace,
					flags,
					visibility
				);
			}

			template <>
			struct RootParameterInfoMap<Brawler::RootSignatures::RootSRVInfo> : public RootParameterInfoMapInstantiation<RootSRVParameter, InitializeSRV>
			{};

			void InitializeUAV(
				D3D12_ROOT_PARAMETER1& rootParam,
				UINT shaderRegister,
				UINT registerSpace,
				D3D12_ROOT_DESCRIPTOR_FLAGS flags,
				D3D12_SHADER_VISIBILITY visibility
			)
			{
				CD3DX12_ROOT_PARAMETER1::InitAsUnorderedAccessView(
					rootParam,
					shaderRegister,
					registerSpace,
					flags,
					visibility
				);
			}

			template <>
			struct RootParameterInfoMap<Brawler::RootSignatures::RootUAVInfo> : public RootParameterInfoMapInstantiation<RootUAVParameter, InitializeUAV>
			{};

			template <>
			struct RootParameterInfoMap<Brawler::RootSignatures::DescriptorTableInfo> : public RootParameterInfoMapInstantiation<DescriptorTableParameter>
			{};

			template <RootSignatureID RSIdentifier>
			concept IsRootSignatureDefined = requires (RootSignatureID x)
			{
				// Make sure that the root parameter enumeration type for this root signature has
				// the COUNT_OR_ERROR value.
				Brawler::RootParamType<RSIdentifier>::COUNT_OR_ERROR;
			};
		}
	}
}

export namespace Brawler
{
	namespace RootSignatures
	{
		template <RootSignatureID RSIdentifier>
			requires IMPL::IsRootSignatureDefined<RSIdentifier>
		class RootSignatureBuilder
		{
		private:
			// A root signature can consist of at most 64 DWORDs.
			static constexpr std::uint32_t MAX_ROOT_SIGNATURE_COST = 64;

		private:
			struct SerializedRootSignatureBlobs
			{
				Microsoft::WRL::ComPtr<ID3DBlob> Version1_0;
				Microsoft::WRL::ComPtr<ID3DBlob> Version1_1;
			};

		public:
			using RootParamType = Brawler::RootParamType<RSIdentifier>;

		public:
			RootSignatureBuilder() = default;

			RootSignatureBuilder(const RootSignatureBuilder& rhs) = delete;
			RootSignatureBuilder& operator=(const RootSignatureBuilder& rhs) = delete;

			RootSignatureBuilder(RootSignatureBuilder&& rhs) noexcept = default;
			RootSignatureBuilder& operator=(RootSignatureBuilder&& rhs) noexcept = default;

			template <Brawler::RootParamType<RSIdentifier> RootParam, typename RootParamInfoType>
				requires (RootParam != Brawler::RootParamType<RSIdentifier>::COUNT_OR_ERROR) && requires (RootParamInfoType x)
			{
				std::unique_ptr<IMPL::I_RootParameter> rootParamPtr{ std::make_unique<typename IMPL::RootParameterInfoMap<RootParamInfoType>::RootParamType>(std::move(x)) };
			}
			void InitializeRootParameter(RootParamInfoType&& par);

			/// <summary>
			/// Adds a static sampler to the root signature.
			/// 
			/// The MSVC states that there is "no performance cost" to using static samplers, and
			/// that if a sampler can be defined as static, then there is no need for the sampler
			/// to be part of a descriptor heap.
			/// 
			/// In other words, if a sampler *CAN* be static, then it *SHOULD* be static.
			/// </summary>
			/// <param name="staticSamplerDesc">
			/// - The description of the static sampler which is to be added to the root signature.
			/// </param>
			void AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC&& staticSamplerDesc);

			FileWriterNode CreateRootSignatureDefinitionFileWriterNode() const;

		private:
			SerializedRootSignatureBlobs CreateSerializedRootSignatures() const;
			void UpdateRootSignatureCost(const std::uint32_t costIncrease);

		private:
			std::array<std::unique_ptr<IMPL::I_RootParameter>, std::to_underlying(Brawler::RootParamType<RSIdentifier>::COUNT_OR_ERROR)> mRootParamArr;
			std::vector<CD3DX12_STATIC_SAMPLER_DESC> mStaticSamplerArr;
			std::uint32_t mRootSignatureCost;
		};
	}
}

// ------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace RootSignatures
	{
		namespace IMPL
		{
			/*
			At the time of writing this, attempting to instead define the non-templated functions
			in a .cpp file results in a critical MSVC failure where CL.exe crashes.

			I haven't pinpointed exactly what is causing the issue, but #including "DxDef.h" in the
			same file as a module implementation unit for Brawler.RootSignatureBuilder causes the
			crash.
			*/

			RootConstantsParameter::RootConstantsParameter(Brawler::RootSignatures::RootConstantsInfo&& paramInfo) :
				mParamInfo(std::move(paramInfo))
			{}

			void RootConstantsParameter::InitializeRootParameter(CD3DX12_ROOT_PARAMETER1& rootParam) const
			{
				rootParam.InitAsConstants(
					mParamInfo.Num32BitValues,
					mParamInfo.ShaderRegister,
					mParamInfo.RegisterSpace,
					mParamInfo.Visibility
				);
			}

			std::string_view RootConstantsParameter::GetRootParameterTypeString() const
			{
				return "Brawler::RootParameters::RootParameterType::ROOT_CONSTANT";
			}

			std::uint32_t RootConstantsParameter::GetRootSignatureCost() const
			{
				// Each 32-bit value is equivalent to one DWORD.
				return mParamInfo.Num32BitValues;
			}

			template <typename ParamInfo>
			RootDescriptorParameter<ParamInfo>::RootDescriptorParameter(ParamInfo&& paramInfo) :
				mParamInfo(std::move(paramInfo))
			{}

			template <typename ParamInfo>
			void RootDescriptorParameter<ParamInfo>::InitializeRootParameter(CD3DX12_ROOT_PARAMETER1& rootParam) const
			{
				IMPL::RootParameterInfoMap<ParamInfo>::INITIALIZATION_FUNCTION(
					rootParam,
					mParamInfo.ShaderRegister,
					mParamInfo.RegisterSpace,
					mParamInfo.Flags,
					mParamInfo.Visibility
				);
			}

			template <typename ParamInfo>
			std::string_view RootDescriptorParameter<ParamInfo>::GetRootParameterTypeString() const
			{
				if constexpr (ParamInfo::ROOT_PARAM_TYPE == Brawler::RootParameters::RootParameterType::CBV)
					return "Brawler::RootParameters::RootParameterType::CBV";

				if constexpr (ParamInfo::ROOT_PARAM_TYPE == Brawler::RootParameters::RootParameterType::SRV)
					return "Brawler::RootParameters::RootParameterType::SRV";

				if constexpr (ParamInfo::ROOT_PARAM_TYPE == Brawler::RootParameters::RootParameterType::UAV)
					return "Brawler::RootParameters::RootParameterType::UAV";
			}

			template <typename ParamInfo>
			std::uint32_t RootDescriptorParameter<ParamInfo>::GetRootSignatureCost() const
			{
				// Each root descriptor is equivalent to two DWORDs, since they are essentially 64-bit 
				// addresses.
				return 2;
			}

			DescriptorTableParameter::DescriptorTableParameter(Brawler::RootSignatures::DescriptorTableInfo&& paramInfo) :
				mParamInfo(std::move(paramInfo))
			{}

			void DescriptorTableParameter::InitializeRootParameter(CD3DX12_ROOT_PARAMETER1& rootParam) const
			{
				rootParam.InitAsDescriptorTable(
					static_cast<std::uint32_t>(mParamInfo.DescriptorRangeArr.size()),
					mParamInfo.DescriptorRangeArr.data(),
					mParamInfo.Visibility
				);
			}

			std::string_view DescriptorTableParameter::GetRootParameterTypeString() const
			{
				return "Brawler::RootParameters::RootParameterType::DESCRIPTOR_TABLE";
			}

			std::uint32_t DescriptorTableParameter::GetRootSignatureCost() const
			{
				// For some reason, descriptor tables cost one DWORD each. The MSDN gives no explanation 
				// as to why this is the case.
				return 1;
			}
		}
	}
}

namespace Brawler
{
	namespace RootSignatures
	{
		template <RootSignatureID RSIdentifier>
			requires IMPL::IsRootSignatureDefined<RSIdentifier>
		template <Brawler::RootParamType<RSIdentifier> RootParam, typename RootParamInfoType>
			requires (RootParam != Brawler::RootParamType<RSIdentifier>::COUNT_OR_ERROR) && requires (RootParamInfoType x)
		{
			std::unique_ptr<IMPL::I_RootParameter> rootParamPtr{ std::make_unique<typename IMPL::RootParameterInfoMap<RootParamInfoType>::RootParamType>(std::move(x)) };
		}
		void RootSignatureBuilder<RSIdentifier>::InitializeRootParameter(RootParamInfoType&& paramInfo)
		{
			using RootParam_T = IMPL::RootParameterInfoMap<RootParamInfoType>::RootParamType;

			if (mRootParamArr[std::to_underlying(RootParam)] != nullptr) [[unlikely]]
				mRootSignatureCost -= mRootParamArr[std::to_underlying(RootParam)]->GetRootSignatureCost();

			std::unique_ptr<IMPL::I_RootParameter> rootParameter{ std::make_unique<RootParam_T>(std::move(paramInfo)) };
			const IMPL::I_RootParameter* const rootParameterPtr = rootParameter.get();

			mRootParamArr[std::to_underlying(RootParam)] = std::move(rootParameter);

			// Update the root signature cost.
			UpdateRootSignatureCost(rootParameterPtr->GetRootSignatureCost());
		}

		template <RootSignatureID RSIdentifier>
			requires IMPL::IsRootSignatureDefined<RSIdentifier>
		void RootSignatureBuilder<RSIdentifier>::AddStaticSampler(CD3DX12_STATIC_SAMPLER_DESC&& staticSamplerDesc)
		{
			mStaticSamplerArr.push_back(std::move(staticSamplerDesc));

			// Static samplers do not have any cost in the size of the root signature.
		}

		template <RootSignatureID RSIdentifier>
			requires IMPL::IsRootSignatureDefined<RSIdentifier>
		FileWriterNode RootSignatureBuilder<RSIdentifier>::CreateRootSignatureDefinitionFileWriterNode() const
		{
			/*
			template <>
			struct RootSignatureDefinition<RootSignatureID::X>
			{
				static constexpr std::array<std::uint8_t, X> SERIALIZED_ROOT_SIGNATURE_VERSION_1_0{...};
				static constexpr std::array<std::uint8_t, X> SERIALIZED_ROOT_SIGNATURE_VERSION_1_1{...};

				using RootParamEnumType = X;

				static constexpr std::array<Brawler::RootParameters::RootParameterType, std::to_underlying(RootParamEnumType::COUNT_OR_ERROR)> ROOT_PARAM_TYPES_ARR{...};
			};
			*/
			SerializedRootSignatureBlobs rootSigBlobs{ CreateSerializedRootSignatures() };

			std::string definitionStr{ "\t\ttemplate <>\n\t\tstruct RootSignatureDefinition<RootSignatureID::" };
			definitionStr += GetRootSignatureIDString<RSIdentifier>();

			// Write out the byte array which makes up the serialized 1.0 root signature.
			definitionStr += ">\n\t\t{\n\t\t\tstatic constexpr std::array<std::uint8_t, ";
			definitionStr += std::to_string(rootSigBlobs.Version1_0->GetBufferSize());
			definitionStr += "> SERIALIZED_ROOT_SIGNATURE_VERSION_1_0{";
			definitionStr += Util::FileWrite::CreateSTDUInt8ArrayContentsStringFromBuffer(std::span<const std::uint8_t>{
				reinterpret_cast<const std::uint8_t*>(rootSigBlobs.Version1_0->GetBufferPointer()),
					rootSigBlobs.Version1_0->GetBufferSize()
			});

			// Write out the byte array which makes up the serialized 1.1 root signature.
			definitionStr += "};\n\t\t\tstatic constexpr std::array<std::uint8_t, ";
			definitionStr += std::to_string(rootSigBlobs.Version1_1->GetBufferSize());
			definitionStr += "> SERIALIZED_ROOT_SIGNATURE_VERSION_1_1{";
			definitionStr += Util::FileWrite::CreateSTDUInt8ArrayContentsStringFromBuffer(std::span<const std::uint8_t>{
				reinterpret_cast<const std::uint8_t*>(rootSigBlobs.Version1_1->GetBufferPointer()),
					rootSigBlobs.Version1_1->GetBufferSize()
			});

			// Write out the root parameter enumeration type.
			definitionStr += "};\n\n\t\t\tusing RootParamEnumType = Brawler::RootParameters::";
			definitionStr += Brawler::RootParameters::GetEnumClassNameString<RootParamType>();
			definitionStr += ";\n\n";

			// Write out the types of all of the root parameters.
			{
				definitionStr += "\t\t\tstatic constexpr std::array<Brawler::RootParameters::RootParameterType, std::to_underlying(RootParamEnumType::COUNT_OR_ERROR)> ROOT_PARAM_TYPES_ARR{";
				
				bool addComma = false;
				for (const auto& rootParam : mRootParamArr)
				{
					if (addComma)
						definitionStr += ",";
					else
						addComma = true;

					assert(rootParam != nullptr);
					definitionStr += rootParam->GetRootParameterTypeString();
				}

				definitionStr += "};\n\t\t};\n\n";
			}
			
			Brawler::FileWriterNode rootSignatureDefinitionNode{};
			rootSignatureDefinitionNode.SetOutputText(std::move(definitionStr));

			return rootSignatureDefinitionNode;
		}

		template <RootSignatureID RSIdentifier>
			requires IMPL::IsRootSignatureDefined<RSIdentifier>
		RootSignatureBuilder<RSIdentifier>::SerializedRootSignatureBlobs RootSignatureBuilder<RSIdentifier>::CreateSerializedRootSignatures() const
		{
			std::array<CD3DX12_ROOT_PARAMETER1, std::to_underlying(Brawler::RootParamType<RSIdentifier>::COUNT_OR_ERROR)> rootParameterArr{};

			for (std::size_t i = 0; i < rootParameterArr.size(); ++i)
			{
				if (mRootParamArr[i] == nullptr) [[unlikely]]
					throw std::runtime_error{ "ERROR: A root parameter was left undefined for a particular root signature!" };

				mRootParamArr[i]->InitializeRootParameter(rootParameterArr[i]);
			}

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{ static_cast<std::uint32_t>(rootParameterArr.size()), rootParameterArr.data(), static_cast<std::uint32_t>(mStaticSamplerArr.size()), mStaticSamplerArr.data(), Brawler::GetRootSignatureFlags<RSIdentifier>() };

			SerializedRootSignatureBlobs rootSigBlobs{};
			Microsoft::WRL::ComPtr<ID3DBlob> errorBlob{};

			CheckHRESULT(D3DX12SerializeVersionedRootSignature(
				&rootSignatureDesc,
				D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_0,
				&(rootSigBlobs.Version1_0),
				&errorBlob
			));

			if (errorBlob != nullptr) [[unlikely]]
				throw std::runtime_error{ std::string{ "ERROR: A particular root signature failed to be serialized under the Root Signature 1.0 standard! The error message is as follows:\n" } + reinterpret_cast<const char*>(errorBlob->GetBufferPointer()) };

			CheckHRESULT(D3DX12SerializeVersionedRootSignature(
				&rootSignatureDesc,
				D3D_ROOT_SIGNATURE_VERSION::D3D_ROOT_SIGNATURE_VERSION_1_1,
				&(rootSigBlobs.Version1_1),
				&errorBlob
			));

			if (errorBlob != nullptr) [[unlikely]]
				throw std::runtime_error{ std::string{ "ERROR: A particular root signature failed to be serialized under the Root Signature 1.1 standard! The error message is as follows:\n" } + reinterpret_cast<const char*>(errorBlob->GetBufferPointer()) };

			return rootSigBlobs;
		}

		template <RootSignatureID RSIdentifier>
			requires IMPL::IsRootSignatureDefined<RSIdentifier>
		void RootSignatureBuilder<RSIdentifier>::UpdateRootSignatureCost(const std::uint32_t costIncrease)
		{
			mRootSignatureCost += costIncrease;

			if (mRootSignatureCost > MAX_ROOT_SIGNATURE_COST) [[unlikely]]
				throw std::runtime_error{ "ERROR: The maximum limit of 64 DWORDs for a root signature has been exceeded!" };
		}
	}
}
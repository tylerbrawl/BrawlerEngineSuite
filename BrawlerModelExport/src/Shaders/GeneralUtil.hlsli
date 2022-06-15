namespace IMPL
{
	// So, HLSL 2021 supports templates, eh? Heh, heh, heh...
	//
	// (Side Note: Am I the only person masochistic enough to bring template meta-programming to HLSL?)
	
	template <typename T>
	struct SaturatedSelectionInfo
	{};
	
	template <>
    struct SaturatedSelectionInfo<uint>
	{
        typedef bool ParameterType;
    };

	template <>
    struct SaturatedSelectionInfo<float>
	{
        typedef bool ParameterType;
    };
	
	template <>
    struct SaturatedSelectionInfo<uint2>
	{
        typedef bool2 ParameterType;
    };
	
	template <>
    struct SaturatedSelectionInfo<float2>
	{
        typedef bool2 ParameterType;
    };
	
	template <>
    struct SaturatedSelectionInfo<uint3>
	{
        typedef bool3 ParameterType;
    };
	
	template <>
    struct SaturatedSelectionInfo<float3>
	{
        typedef bool3 ParameterType;
    };
	
	template <>
    struct SaturatedSelectionInfo<uint4>
	{
        typedef bool4 ParameterType;
    };
	
	template <>
    struct SaturatedSelectionInfo<float4>
	{
        typedef bool4 ParameterType;
    };
}

template <typename T>
T SaturatedSelection(in const IMPL::SaturatedSelectionInfo<T>::ParameterType boolVector)
{
	return select(boolVector, 1, 0);
}

uint SelectComponent(in const uint inputVector, in const uint index)
{
    return (index == 0 ? inputVector : 0);
}

float SelectComponent(in const float inputVector, in const uint index)
{
    return (index == 0 ? inputVector : 0.0f);
}

uint SelectComponent(in const uint2 inputVector, in const uint index)
{
    return dot(inputVector, uint2(select(index == 0, 1, 0), select(index == 1, 1, 0)));
}

float SelectComponent(in const float2 inputVector, in const uint index)
{
    return dot(inputVector, float2(select(index == 0, 1.0f, 0.0f), select(index == 1, 1.0f, 0.0f)));
}

uint SelectComponent(in const uint3 inputVector, in const uint index)
{
    return dot(inputVector, uint3(select(index == 0, 1, 0), select(index == 1, 1, 0), select(index == 2, 1, 0)));
}

float SelectComponent(in const float3 inputVector, in const uint index)
{
    return dot(inputVector, float3(select(index == 0, 1.0f, 0.0f), select(index == 1, 1.0f, 0.0f), select(index == 2, 1.0f, 0.0f)));
}

uint SelectComponent(in const uint4 inputVector, in const uint index)
{
    return dot(inputVector, uint4(select(index == 0, 1, 0), select(index == 1, 1, 0), select(index == 2, 1, 0), select(index == 3, 1, 0)));
}

float SelectComponent(in const float4 inputVector, in const uint index)
{
    return dot(inputVector, float4(select(index == 0, 1.0f, 0.0f), select(index == 1, 1.0f, 0.0f), select(index == 2, 1.0f, 0.0f), select(index == 3, 1.0f, 0.0f)));
}

template <typename T>
IMPL::SelectComponentInfo<T>::ReturnType SelectComponent(in const T inputVector, in const uint index)
{
	return IMPL::SelectComponentInfo<T>::SelectComponentIMPL(inputVector, index);
}

#ifndef __cplusplus
template <typename ReturnType, typename ParamType>
ReturnType static_cast(in const ParamType param)
{
    return (ReturnType) param;
}
#endif
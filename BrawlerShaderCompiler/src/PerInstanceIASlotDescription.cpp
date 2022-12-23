module;
#include <vector>
#include <span>
#include <string_view>
#include "DxDef.h"

module Brawler.PerInstanceIASlotDescription;

namespace Brawler
{
	PerInstanceIASlotDescription::PerInstanceIASlotDescription(const std::uint32_t instanceDataStepRate) :
		mInputElementArr(),
		mInstanceDataStepRate(instanceDataStepRate)
	{}

	void PerInstanceIASlotDescription::AddPerInstanceInputElement(const std::string_view semanticName, const DXGI_FORMAT format)
	{
		assert(std::ranges::find_if(mInputElementArr, [semanticName] (const PerInstanceInputElement& inputElement) { return (inputElement.SemanticName == semanticName); }) == mInputElementArr.end() && "ERROR: An attempt was made to specify more than one input element with the same semantic name in a given IA slot input layout!");
		
		mInputElementArr.push_back(PerInstanceInputElement{
			.SemanticName{ semanticName },
			.Format = format
		});
	}

	void PerInstanceIASlotDescription::SetInstanceDataStepRate(const std::uint32_t instanceDataStepRate)
	{
		mInstanceDataStepRate = instanceDataStepRate;
	}

	std::uint32_t PerInstanceIASlotDescription::GetInstanceDataStepRate() const
	{
		return mInstanceDataStepRate;
	}

	std::span<const PerInstanceIASlotDescription::PerInstanceInputElement> PerInstanceIASlotDescription::GetInputElementSpan() const
	{
		return std::span<const PerInstanceInputElement>{ mInputElementArr };
	}
}
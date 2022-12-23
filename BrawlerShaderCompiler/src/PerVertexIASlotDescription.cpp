module;
#include <vector>
#include <span>
#include <string_view>
#include <algorithm>
#include <cassert>
#include "DxDef.h"

module Brawler.PerVertexIASlotDescription;

namespace Brawler
{
	void PerVertexIASlotDescription::AddPerVertexInputElement(const std::string_view semanticName, const DXGI_FORMAT format)
	{
		assert(std::ranges::find_if(mInputElementArr, [semanticName] (const PerVertexInputElement& inputElement) { return (inputElement.SemanticName == semanticName); }) == mInputElementArr.end() && "ERROR: An attempt was made to specify more than one input element with the same semantic name in a given IA slot input layout!");
		
		mInputElementArr.push_back(PerVertexInputElement{
			.SemanticName{ semanticName },
			.Format = format
		});
	}

	std::span<const PerVertexIASlotDescription::PerVertexInputElement> PerVertexIASlotDescription::GetInputElementSpan() const
	{
		return std::span<const PerVertexInputElement>{ mInputElementArr };
	}
}
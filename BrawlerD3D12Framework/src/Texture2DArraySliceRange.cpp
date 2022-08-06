module;
#include <cstdint>
#include <cassert>
#include "DxDef.h"

module Brawler.D3D12.Texture2DArray;
import Util.General;

namespace Brawler
{
	namespace D3D12
	{
		Texture2DArraySliceRange::Texture2DArraySliceRange(Texture2DArray& tex2DArr, const std::uint32_t firstSliceIndex, const std::uint32_t numSlices) :
			mTexArrayPtr(&tex2DArr),
			mFirstSliceIndex(firstSliceIndex),
			mNumSlices(numSlices)
		{
			assert((firstSliceIndex + numSlices) <= tex2DArr.GetResourceDescription().DepthOrArraySize && "ERROR: An invalid/out-of-bounds range of array slices was specified when creating a Texture2DArraySliceRange instance!");
		}

		Texture2DArray& Texture2DArraySliceRange::GetTexture2DArray()
		{
			assert(mTexArrayPtr != nullptr);
			return *mTexArrayPtr;
		}

		const Texture2DArray& Texture2DArraySliceRange::GetTexture2DArray() const
		{
			assert(mTexArrayPtr != nullptr);
			return *mTexArrayPtr;
		}

		Texture2DArraySlice Texture2DArraySliceRange::GetArraySlice(const std::uint32_t relativeSliceIndex)
		{
			assert(relativeSliceIndex < mNumSlices && "ERROR: An attempt was made to get a Texture2DArraySlice by calling Texture2DArraySliceRange::GetArraySlice(), but the specified slice index was outside of the boundaries of the range!");
			return Texture2DArraySlice{ GetTexture2DArray(), (mFirstSliceIndex + relativeSliceIndex) };
		}
	}
}
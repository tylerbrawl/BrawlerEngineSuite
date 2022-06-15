module;
#include <utility>

module Brawler.I_MaterialDefinition;

namespace Brawler
{
	I_MaterialDefinition::I_MaterialDefinition(ImportedMesh&& mesh) :
		mMesh(std::move(mesh))
	{}

	const ImportedMesh& I_MaterialDefinition::GetImportedMesh() const
	{
		return mMesh;
	}
}
module;

export module Brawler.MeshAttributePointerDescriptor;
import Brawler.FilePathHash;
import Brawler.MeshAttributeID;

export namespace Brawler
{
	struct MeshAttributePointerDescriptor
	{
		/// <summary>
		/// This is the FilePathHash which can be used to locate the MAP data within
		/// the BPK archive once the entire model has been archived.
		/// </summary>
		FilePathHash MAPFilePathHash;

		/// <summary>
		/// This describes the type of the MAP. Different examples include vertex/index
		/// buffers and materials.
		/// </summary>
		MeshAttributeID Identifier;
	};
}
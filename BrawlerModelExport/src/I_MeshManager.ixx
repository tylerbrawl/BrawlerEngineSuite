module;
#include <assimp/mesh.h>
#include <fstream>

export module Brawler.I_MeshManager;
import Brawler.MeshTypeID;

export namespace Brawler
{
	class I_MeshManager
	{
	protected:
		I_MeshManager() = default;

	public:
		virtual ~I_MeshManager() = default;

		/// <summary>
		/// This is automatically called by the ModelResolver to inform the I_MeshManager instance
		/// that it should begin processing the relevant mesh data. 
		/// 
		/// This delay is done in order to ensure that threads which create an I_MeshManager instance 
		/// do not accidentally trigger a long wait for mesh processing to complete, as this could 
		/// prevent other CPU jobs from being scheduled and potentially lead to thread starvation.
		/// </summary>
		virtual void BeginInitialization() = 0;

		/// <summary>
		/// Retrieves the Brawler::MeshTypeID of this derived class of I_MeshManager. Derived
		/// classes should implement this to return a unique identifier across all derived classes.
		/// </summary>
		/// <returns>
		/// This function returns the Brawler::MeshTypeID of this derived class of I_MeshManager.
		/// </returns>
		virtual MeshTypeID GetMeshTypeID() const = 0;

		//virtual void SerializeMeshData(std::ofstream& fileStream) const = 0;
	};
}
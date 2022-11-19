module;
#include <atomic>
#include <cstdint>
#include <span>

export module Brawler.I_MaterialDefinition;
import Brawler.SceneTextureHandles;
import Brawler.FilePathHash;

export namespace Brawler 
{
	class I_MaterialDefinition
	{
	protected:
		I_MaterialDefinition() = default;

	public:
		virtual ~I_MaterialDefinition() = default;

		I_MaterialDefinition(const I_MaterialDefinition& rhs) = delete;
		I_MaterialDefinition& operator=(const I_MaterialDefinition& rhs) = delete;

		I_MaterialDefinition(I_MaterialDefinition&& rhs) noexcept = default;
		I_MaterialDefinition& operator=(I_MaterialDefinition&& rhs) noexcept = default;

		virtual std::span<const FilePathHash> GetDependentSceneTextureFilePathHashSpan() const = 0;
		
		/// <summary>
		/// This function is called on a given frame whenever any of the scene textures associated
		/// with an I_MaterialDefinition instance are updated (e.g., as a result of mip streaming).
		/// 
		/// Regardless of how many scene textures are changed within a frame, this function is only
		/// called once per dependent I_MaterialDefinition instance per frame.
		/// 
		/// The intent is that derived classes will implement this function to update the relevant
		/// data in GPUScene buffers as a result of changes in texture data. For instance, one
		/// might implement this function to update the bindless SRV indices associated with a
		/// material to account for new mip levels being streamed in for the same texture.
		/// </summary>
		virtual void UpdateGPUSceneMaterialDescriptor() const = 0;

		/// <summary>
		/// Derived classes should implement this function to return the index within
		/// the relevant GPUScene buffer for this I_MaterialDefinition instance. Note that this
		/// is the index within the buffer containing the material description data.
		/// 
		/// For instance, the StandardMaterialDefinition class overrides this function to return
		/// the index within the MaterialDescriptor GPUSceneBuffer for a given instance.
		/// 
		/// To be perfectly clear, this is *NOT* a bindless SRV index! Each material class should
		/// have its own GPUSceneBuffer, and this function should return an index into *that* buffer.
		/// </summary>
		/// <returns>
		/// Derived classes should implement this function to return the index within
		/// the relevant GPUScene buffer for this I_MaterialDefinition instance.
		/// </returns>
		virtual std::uint32_t GetGPUSceneBufferIndex() const = 0;

		void NotifyForDeletion();
		bool ReadyForDeletion() const;

	private:
		std::atomic<bool> mReadyForDeletion;
	};
}
module;
#include <atomic>

export module Brawler.I_Asset;
import Brawler.AssetTypeID;
import Brawler.FilePathHash;
import Brawler.JobPriority;

export namespace Brawler
{
	class AssetManager;
	struct AssetDataLoadContext;
	struct AssetDataUnloadContext;
}

namespace Brawler
{
	class AssetDependencyGroup;
	class PersistentAssetDependencyGroup;
}

export namespace Brawler
{
	class I_Asset
	{
	private:
		friend class AssetManager;
		friend class AssetDependencyGroup;
		friend class PersistentAssetDependencyGroup;

	protected:
		explicit I_Asset(FilePathHash&& pathHash);

	public:
		virtual ~I_Asset() = default;

		I_Asset(const I_Asset& rhs) = delete;
		I_Asset& operator=(const I_Asset& rhs) = delete;

		I_Asset(I_Asset&& rhs) noexcept = default;
		I_Asset& operator=(I_Asset&& rhs) noexcept = default;

		/// <summary>
		/// Determines whether or not this I_Asset is considered streamable. A streamable asset
		/// is one which is checked every update to see if data needs to be loaded or unloaded.
		/// 
		/// This can be a drain on performance for levels with many assets. Thus, if an asset
		/// only needs its data to be loaded once and does not participate in streaming, then
		/// it should inherit from I_PersistentAsset, rather than I_Asset.
		/// 
		/// Derived classes should *NOT* override this function. Instead, they should inherit
		/// from I_PersistentAsset, as described above.
		/// </summary>
		/// <returns>
		/// This function returns true if this I_Asset is considered streamable and false
		/// otherwise. By default, this function returns true.
		/// </returns>
		virtual bool IsStreamable() const;

		/// <summary>
		/// This function should be overridden by derived classes to make requests for loading
		/// data to the AssetManager as needed.
		/// </summary>
		virtual void UpdateAssetData() = 0;

		/// <summary>
		/// This function is called by the AssetManager to let an I_Asset instance know that
		/// it is time to load at least some of its required data. The AssetManager itself is 
		/// *NOT* responsible for managing the lifetime of the data; that is the responsibility 
		/// of the I_Asset instances.
		/// 
		/// The suggestion is for derived classes to both store their own data and offer an
		/// API for interacting with it once it is loaded.
		/// 
		/// WARNING: This function is called from multiple threads, and *MUST* be made
		/// thread-safe!
		/// </summary>
		/// <param name="context">
		/// - This structure contains information which will likely prove useful when loading
		/// asset data.
		/// </param>
		virtual void ExecuteAssetDataLoad(const AssetDataLoadContext& context) = 0;

		/// <summary>
		/// This function is called by the AssetManager to let an I_Asset instance know that
		/// it is time to unload at least some of its loaded asset data. Derived classes
		/// should *NOT* delete any asset data unless and until this function is called!
		/// 
		/// Not only will waiting for the right moment to deallocate memory help to reduce
		/// stuttering, but it may also be necessary if other objects are referring to the
		/// data.
		/// 
		/// WARNING: This function is called from multiple threads, and *MUST* be made
		/// thread-safe!
		/// </summary>
		/// <param name="context">
		/// - This structure contains information which will likely prove useful when unloading
		/// asset data.
		/// </param>
		virtual void ExecuteAssetDataUnload(const AssetDataUnloadContext& context) = 0;

		/// <summary>
		/// Derived classes should overload this function to indicate that a particular resource
		/// has been fully loaded, i.e., that an instance of the class has loaded all of its
		/// data and will not send any more asset data load requests until at least some of its 
		/// asset data has been unloaded.
		/// </summary>
		/// <returns>
		/// Derived classes should implement this function to return true if an instance of it
		/// is fully loaded and false otherwise. See the summary for what constitutes a "fully-loaded
		/// asset."
		/// </returns>
		virtual bool IsLoaded() const = 0;

		/// <summary>
		/// Retrieves the AssetTypeID for this type. Derived classes of type T *MUST* override this 
		/// to return the same value as IMPL::AssetTypeMap<T>::TYPE_ID (see AssetTypeMap.ixx).
		/// </summary>
		/// <returns>
		/// The function returns the AssetTypeID for this I_Asset instance.
		/// </returns>
		virtual AssetTypeID GetAssetTypeID() const = 0;

	protected:
		/// <summary>
		/// This function submits a request to the AssetManager to load numBytesToLoad bytes of
		/// data. The program will assert in Debug builds if this request is made during any
		/// time other than when the AssetManager allows it.
		/// </summary>
		/// <param name="numBytesToLoad">
		/// - The size, in bytes, of the data which is to be loaded. When possible, try to
		///   split large requests into many small requests; this allows for more efficient
		///   use of system resources.
		/// </param>
		/// <param name="priority">
		/// - The priority which this request has. Load requests with a higher priority are
		///   fulfilled before those with a lower priority.
		/// 
		///	  NOTE: Load requests with JobPriority::CRITICAL priority are guaranteed to be
		///   fulfilled during the next asset dependency update.
		/// </param>
		void SubmitAssetDataLoadRequest(const std::size_t numBytesToLoad, const JobPriority priority = JobPriority::NORMAL);

		/// <summary>
		/// This function submits a request to the AssetManager to unload numBytesToUnload bytes
		/// of data. The program will assert in Debug builds if this request is made during any
		/// time other than when the AssetManager allows it.
		/// </summary>
		/// <param name="numBytesToUnload">
		/// - The size, in bytes, of the data which is to be unloaded. When possible, try to
		///   split large requests into many small requests; this allows for more efficient
		///   use of system resources.
		/// </param>
		/// <param name="priority">
		/// - The priority which this request has. Unload requests with a higher priority are
		///   fulfilled before those with a lower priority.
		/// 
		///   NOTE: Unload requests with JobPriority::CRITICAL priority are guaranteed to be
		///   fulfilled during the next asset dependency update.
		/// </param>
		void SubmitAssetDataUnloadRequest(const std::size_t numBytesToUnload, const JobPriority priority = JobPriority::NORMAL);

	public:
		FilePathHash GetFilePathHash() const;

		AssetManager& GetAssetManager();
		const AssetManager& GetAssetManager() const;

	private:
		/// <summary>
		/// This is an implementation function which is used to ensure that asset data load and
		/// unload requests are only submitted once per asset dependency update. This check is
		/// thread-safe (i.e., if requests are made on one thread, then they will never be made
		/// again on ANY thread for the remainder of that asset dependency update).
		/// </summary>
		void UpdateAssetDataIMPL();

		void SetOwningManager(AssetManager& owningManager);

	private:
		FilePathHash mPathHash;
		AssetManager* mOwningManager;
		std::atomic<std::uint64_t> mUpdateTickCounter;
	};
}
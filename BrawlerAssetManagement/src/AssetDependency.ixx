module;
#include <memory>
#include <vector>
#include <concepts>

export module Brawler.AssetManagement.AssetDependency;

export namespace Brawler
{
	namespace AssetManagement
	{
		class I_AssetLoader;
	}
}

export namespace Brawler
{
	namespace AssetManagement
	{
		class AssetDependency
		{
		public:
			AssetDependency() = default;

			AssetDependency(const AssetDependency& rhs) = delete;
			AssetDependency& operator=(const AssetDependency& rhs) = delete;

			AssetDependency(AssetDependency&& rhs) noexcept = default;
			AssetDependency& operator=(AssetDependency&& rhs) noexcept = default;

			template <typename DerivedType, typename... Args>
				requires std::derived_from<DerivedType, I_AssetLoader> && requires (Args&&... args)
			{
				DerivedType{ std::forward<Args>(args)... }
			}
			void CreateAssetLoaderDependency(Args&&... args);

			/// <summary>
			/// Merges the I_AssetLoader instances contained within dependency into this
			/// AssetDependency instance.
			/// 
			/// The order in which I_AssetLoader instances are completed is *UNDEFINED*,
			/// regardless of whether they were merged into an AssetDependency instance or created
			/// directly within one. This is a natural consequence of the multithreaded nature
			/// of asset loading.
			/// 
			/// To be perfectly correct, it would be possible to establish some form of dependency
			/// ordering between I_AssetLoader instances, even if assets are loaded concurrently.
			/// However, we deem this to be a poor design choice, since it both adds complexity and
			/// limits parallelism. If a set of steps must be executed sequentially, then they should
			/// all be done within the same I_AssetLoader instance.
			/// 
			/// If this type of behavior really is needed, then one solution might be to create
			/// and submit AssetDependency instances from within I_AssetLoader instances, once they
			/// are ready.
			/// </summary>
			/// <param name="dependency">
			/// - The AssetDependency instance whose I_AssetLoader instances are to be merged into
			///   this AssetDependency instance.
			/// </param>
			void MergeAssetDependency(AssetDependency&& dependency);

		private:
			std::vector<std::unique_ptr<I_AssetLoader>> mAssetLoaderDependencyArr;
		};
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace AssetManagement
	{
		template <typename DerivedType, typename... Args>
			requires std::derived_from<DerivedType, I_AssetLoader> && requires (Args&&... args)
		{
			DerivedType{ std::forward<Args>(args)... };
		}
		void AssetDependency::CreateAssetLoaderDependency(Args&&... args)
		{
			mAssetLoaderDependencyArr.emplace_back(std::make_unique<DerivedType>(std::forward<Args>(args)...));
		}
	}
}
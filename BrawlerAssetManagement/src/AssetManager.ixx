module;

export module Brawler.AssetManagement.AssetManager;

export namespace Brawler
{
	namespace AssetManagement
	{
		class AssetManager final
		{
		private:
			AssetManager();

		public:
			~AssetManager() = default;

			AssetManager(const AssetManager& rhs) = delete;
			AssetManager& operator=(const AssetManager& rhs) = delete;

			AssetManager(AssetManager&& rhs) noexcept = default;
			AssetManager& operator=(AssetManager&& rhs) noexcept = default;

			static AssetManager& GetInstance();

		private:

		};
	}
}
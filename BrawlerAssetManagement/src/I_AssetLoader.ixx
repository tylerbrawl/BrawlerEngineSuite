module;

export module Brawler.AssetManagement.I_AssetLoader;

export namespace Brawler
{
	namespace AssetManagement
	{
		class I_AssetLoader
		{
		protected:
			I_AssetLoader() = default;

		public:
			virtual ~I_AssetLoader() = default;

			I_AssetLoader(const I_AssetLoader& rhs) = delete;
			I_AssetLoader& operator=(const I_AssetLoader& rhs) = delete;

			I_AssetLoader(I_AssetLoader&& rhs) noexcept = default;
			I_AssetLoader& operator=(I_AssetLoader&& rhs) noexcept = default;

		private:

		};
	}
}
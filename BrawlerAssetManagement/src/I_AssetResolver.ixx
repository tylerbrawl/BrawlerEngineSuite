module;

export module Brawler.AssetManagement.I_AssetResolver;

export namespace Brawler
{
	namespace AssetManagement
	{
		class I_AssetResolver
		{
		protected:
			I_AssetResolver() = default;

		public:
			virtual ~I_AssetResolver() = default;

			I_AssetResolver(const I_AssetResolver& rhs) = delete;
			I_AssetResolver& operator=(const I_AssetResolver& rhs) = delete;

			I_AssetResolver(I_AssetResolver&& rhs) noexcept = default;
			I_AssetResolver& operator=(I_AssetResolver&& rhs) noexcept = default;

		private:

		};
	}
}
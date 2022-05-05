module;
#include <concepts>

export module Brawler.AssetHandle;

export namespace Brawler
{
	class I_Asset;
	class AssetManager;
}

export namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, I_Asset>
	class AssetHandle
	{
	private:
		using ValueType = std::decay_t<T>;

	private:
		friend class AssetManager;

	private:
		template <typename U = T>
		explicit AssetHandle(std::decay_t<U>& asset);

	public:
		~AssetHandle() = default;

		AssetHandle(const AssetHandle<T>& rhs) = default;
		AssetHandle& operator=(const AssetHandle<T>& rhs) = default;

		AssetHandle(AssetHandle<T>&& rhs) noexcept = default;
		AssetHandle& operator=(AssetHandle<T>&& rhs) noexcept = default;

		ValueType& operator*();
		const ValueType& operator*() const;
		ValueType* operator->();
		const ValueType* operator->() const;

	private:
		ValueType* mAssetPtr;
	};
}

// -----------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, I_Asset>
	template <typename U>
	AssetHandle<T>::AssetHandle(std::decay_t<U>& asset) :
		mAssetPtr(&asset)
	{}

	template <typename T>
		requires std::derived_from<T, I_Asset>
	typename AssetHandle<T>::ValueType& AssetHandle<T>::operator*()
	{
		return *mAssetPtr;
	}

	template <typename T>
		requires std::derived_from<T, I_Asset>
	typename const AssetHandle<T>::ValueType& AssetHandle<T>::operator*() const
	{
		return *mAssetPtr;
	}

	template <typename T>
		requires std::derived_from<T, I_Asset>
	typename AssetHandle<T>::ValueType* AssetHandle<T>::operator->()
	{
		return mAssetPtr;
	}

	template <typename T>
		requires std::derived_from<T, I_Asset>
	typename const AssetHandle<T>::ValueType* AssetHandle<T>::operator->() const
	{
		return mAssetPtr;
	}
}
module;

export module Brawler.ModelLoader;
import Brawler.FilePathHash;

export namespace Brawler
{
	class ModelLoader final
	{
	private:
		ModelLoader() = default;

	public:
		~ModelLoader() = default;

		ModelLoader(const ModelLoader& rhs) = delete;
		ModelLoader& operator=(const ModelLoader& rhs) = delete;

		ModelLoader(ModelLoader&& rhs) noexcept = delete;
		ModelLoader& operator=(ModelLoader&& rhs) noexcept = delete;

		static ModelLoader& GetInstance();


	};
}
module;
#include <memory>

export module Brawler.ModelHandle;
import Brawler.Model;

export namespace Brawler
{
	class ModelHandle
	{
	public:
		ModelHandle() = default;
		explicit ModelHandle(std::shared_ptr<Model> sharedModelPtr);

		ModelHandle(const ModelHandle& rhs) = delete;
		ModelHandle& operator=(const ModelHandle& rhs) = delete;

		ModelHandle(ModelHandle&& rhs) noexcept = default;
		ModelHandle& operator=(ModelHandle&& rhs) noexcept = default;

		const Model& GetModel() const;

	private:
		std::shared_ptr<Model> mModelPtr;
	};
}
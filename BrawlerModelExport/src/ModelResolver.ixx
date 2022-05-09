module;
#include <memory>
#include <vector>

export module Brawler.ModelResolver;
import Brawler.I_ModelResolverComponent;

export namespace Brawler
{
	class ModelResolver
	{
	public:
		ModelResolver() = default;

		ModelResolver(const ModelResolver& rhs) = delete;
		ModelResolver& operator=(const ModelResolver& rhs) = delete;

		ModelResolver(ModelResolver&& rhs) noexcept = default;
		ModelResolver& operator=(ModelResolver&& rhs) noexcept = default;

		void CreateModelResolverComponents();
		void Update();

		bool IsModelSerialized() const;

	private:
		void CreateMeshManager();
		void CreateMaterialManager();

	private:
		std::vector<std::unique_ptr<I_ModelResolverComponent>> mComponentPtrArr;
	};
}
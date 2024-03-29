module;
#include <memory>
#include <vector>

export module Brawler.ModelResolver;
import Brawler.LODResolver;

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

		void Initialize();

		void Update();
		bool IsReadyForSerialization() const;

		void SerializeModelData() const;

	private:
		void InitializeLODResolvers();

	private:
		std::vector<std::unique_ptr<LODResolver>> mLODResolverPtrArr;
	};
}
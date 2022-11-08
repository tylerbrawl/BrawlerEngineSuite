module;

export module Brawler.MaterialDefinition;

export namespace Brawler 
{
	class MaterialDefinition
	{
	public:
		MaterialDefinition() = default;

		MaterialDefinition(const MaterialDefinition& rhs) = delete;
		MaterialDefinition& operator=(const MaterialDefinition& rhs) = delete;

		MaterialDefinition(MaterialDefinition&& rhs) noexcept = default;
		MaterialDefinition& operator=(MaterialDefinition&& rhs) noexcept = default;

	private:

	};
}
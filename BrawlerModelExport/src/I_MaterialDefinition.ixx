module;

export module Brawler.I_MaterialDefinition;
import Brawler.MaterialID;

export namespace Brawler
{
	class I_MaterialDefinition
	{
	protected:
		I_MaterialDefinition() = default;

	public:
		virtual ~I_MaterialDefinition() = default;

		virtual MaterialID GetMaterialID() const = 0;
	};
}
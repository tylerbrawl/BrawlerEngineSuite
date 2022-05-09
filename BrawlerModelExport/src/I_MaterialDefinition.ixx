module;

export module Brawler.I_MaterialDefinition;
import Brawler.MaterialID;
import Brawler.MeshAttributePointerDescriptor;

export namespace Brawler
{
	class I_MaterialDefinition
	{
	protected:
		I_MaterialDefinition() = default;

	public:
		virtual ~I_MaterialDefinition() = default;

		virtual void Update() = 0;
		virtual bool IsMAPSerialized() const = 0;
		virtual MeshAttributePointerDescriptor GetMAPDescriptor() const;

		virtual MaterialID GetMaterialID() const = 0;
	};
}
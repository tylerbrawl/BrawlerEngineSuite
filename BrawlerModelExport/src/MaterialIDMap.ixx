module;

export module Brawler.MaterialIDMap;
import Brawler.MaterialID;

export namespace Brawler
{
	class OpaqueMaterialDefinition;
}

namespace Brawler
{
	namespace IMPL
	{
		template <typename T>
		struct MaterialIDMapInstantiation
		{
			using Type = T;
		};

		template <Brawler::MaterialID ID>
		struct MaterialIDMap
		{
			static_assert(sizeof(ID) != sizeof(ID), "ERROR: A MaterialIDMap was never specified for this MaterialID! (See MaterialIDMap.ixx.)");
		};

		template <>
		struct MaterialIDMap<Brawler::MaterialID::OPAQUE> : public MaterialIDMapInstantiation<Brawler::OpaqueMaterialDefinition>
		{};

		// -------------------------------------------------------------------------------------------------------------------------

		template <Brawler::MaterialID ID_>
		struct MaterialTypeMapInstantiation
		{
			static constexpr Brawler::MaterialID ID = ID_;
		};

		template <typename T>
		struct MaterialTypeMap
		{
			static_assert(sizeof(T) != sizeof(T), "ERROR: A MaterialTypeMap was never specified for this I_MaterialDefinition! (See MaterialIDMap.ixx.)");
		};

		template <>
		struct MaterialTypeMap<Brawler::OpaqueMaterialDefinition> : public MaterialTypeMapInstantiation<Brawler::MaterialID::OPAQUE>
		{};
	}
}

export namespace Brawler
{
	template <MaterialID ID>
	using MaterialDefinitionType = IMPL::MaterialIDMap<ID>::Type;

	template <typename T>
	consteval Brawler::MaterialID GetMaterialID()
	{
		return IMPL::MaterialTypeMap<T>::ID;
	}
}
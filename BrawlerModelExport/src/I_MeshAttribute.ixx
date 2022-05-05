module;
#include <vector>

export module Brawler.I_MeshAttribute;

export namespace Brawler
{
	struct SerializationContext;
}

export namespace Brawler
{
	class I_MeshAttribute
	{
	protected:
		I_MeshAttribute() = default;

	public:
		virtual ~I_MeshAttribute() = default;

		/// <summary>
		/// Derived classes should implement this function to write out their mesh
		/// attribute data in the following format:
		/// 
		///   - The MeshAttributeID of this derived type.
		/// 
		///   - The actual data associated with this I_MeshAttribute instance.
		/// 
		/// It is important to note that derived classes *MUST* write out their
		/// identifier. The std::vector returned by this function should contain all of
		/// this data.
		/// </summary>
		/// <param name="context">
		/// - A structure containing data which may prove useful during the serialization
		///   of an I_MeshAttribute's data.
		/// </param>
		/// <returns>
		/// Derived classes should have this function return a std::vector containing a
		/// serialized form of the attribute's data. See the summary for what needs to
		/// be included.
		/// </returns>
		virtual std::vector<std::uint8_t> SerializeAttributeData(const SerializationContext& context) const = 0;
	};
}
module;

export module Brawler.I_PSOFieldResolver;
import Brawler.FileWriterNode;

export namespace Brawler
{
	class I_PSOFieldResolver
	{
	protected:
		I_PSOFieldResolver() = default;

	public:
		virtual ~I_PSOFieldResolver() = default;

		I_PSOFieldResolver(const I_PSOFieldResolver& rhs) = default;
		I_PSOFieldResolver& operator=(const I_PSOFieldResolver& rhs) = default;

		I_PSOFieldResolver(I_PSOFieldResolver&& rhs) noexcept = default;
		I_PSOFieldResolver& operator=(I_PSOFieldResolver&& rhs) noexcept = default;

		/// <summary>
		/// Some parts of a PSO description can - and should - be inlined as a part of
		/// the PSODefinition. Derived classes can override this function to return a
		/// FileWriterNode instance whose cumulative text contents will be written into
		/// a particular PSODefinition.
		/// 
		/// For instance, the bytecode of a shader can be written out into the PSODefinition.
		/// This makes sense because PSOs will tend to have different shaders. However, root
		/// signatures are more likely to be shared between PSOs; thus, to avoid code bload, 
		/// writing them out many times into different PSODefinitions is not recommended.
		/// </summary>
		/// <returns>
		/// Derived classes can override this function to return a FileWriterNode which
		/// contains text which will be added to a PSODefinition. By default, a
		/// FileWriterNode with no children and no text to write is returned.
		/// </returns>
		virtual FileWriterNode CreatePSODefinitionFileWriterNode() const
		{
			return FileWriterNode{};
		}

		/// <summary>
		/// We like to define as much of a PSO as possible offline. However, there are some
		/// parts of a PSO's description which simply cannot be filled out at compile time.
		/// For these, we need to do some additional work at runtime. Derived classes can
		/// override this function to return a FileWriterNode instance whose contents will
		/// be written into the body of the PSODefinition::ExecuteRuntimePSOResolution()
		/// function of a particular PSODefinition.
		/// 
		/// For instance, root signature objects need to be created at runtime, so it would
		/// make sense to add a statement which modifies the ID3D12RootSignature* of a
		/// PSO's description to point to an object stored in some type of database.
		/// </summary>
		/// <returns>
		/// Derived classes can override this function to return a FileWriterNode which
		/// contains text which will be added to the body of the
		/// PSODefinition::ExecuteRuntimePSOResolution() function of a particular
		/// PSODefinition. By default, a FileWriterNode which no children and no text
		/// to write is returned.
		/// </returns>
		virtual FileWriterNode CreateRuntimePSOResolutionFileWriterNode() const
		{
			return FileWriterNode{};
		}
	};
}
module;
#include <memory>
#include <vector>
#include <string>
#include <fstream>

export module Brawler.FileWriterNode;

export namespace Brawler
{
	class FileWriterNode
	{
	public:
		FileWriterNode() = default;

		FileWriterNode(const FileWriterNode& rhs) = delete;
		FileWriterNode& operator=(const FileWriterNode& rhs) = delete;

		FileWriterNode(FileWriterNode&& rhs) noexcept = default;
		FileWriterNode& operator=(FileWriterNode&& rhs) noexcept = default;

		/// <summary>
		/// Sets the text which will be written out by this FileWriterNode instance.
		/// Writing text is only valid if the FileWriterNode instance does not have any
		/// children.
		/// 
		/// NOTE: In Debug builds, the program will assert if this function is called
		/// after child nodes were added to this FileWriterNode instance.
		/// </summary>
		/// <param name="outputText">
		/// - The text which this FileWriterNode will write out.
		/// </param>
		void SetOutputText(std::string&& outputText);

		/// <summary>
		/// Moves the specified FileWriterNode instance into this FileWriterNode instance
		/// as a child. The contents of child nodes are written out in the order in which
		/// they are added to a FileWriterNode instance. A FileWriterNode instance can only
		/// have children if it is not writing out any text itself.
		/// 
		/// NOTE: In Debug builds, the program will assert if this function is called
		/// after the FileWriterNode was given text to write out.
		/// </summary>
		/// <param name="childNode">
		/// - The unique instance of the FileWriterNode which will be added to this
		///   FileWriterNode instance as a child.
		/// </param>
		void AddChildNode(FileWriterNode&& childNode);

		/// <summary>
		/// Writes out the contents of this FileWriterNode instance. The actual action
		/// taken differs depending on whether this FileWriterNode instance has any
		/// children or not.
		/// 
		///   - If this FileWriterNode instance *DOES* have children, then the contents
		///     of its child nodes are written out in the order in which they were added
		///     to this FileWriterNode instance via calls to FileWriterNode::AddChildNode().
		/// 
		///   - If this FileWriterNode instance does *NOT* have children, then whatever
		///     text it was assigned in a call to FileWriterNode::SetOutputText() is
		///     written out to the file specified by fileStream.
		/// </summary>
		/// <param name="fileStream">
		/// - The file to which the text of either this FileWriterNode instance or its
		///   children nodes will be appended.
		/// </param>
		void WriteOutputText(std::ofstream& fileStream) const;

	private:
		/// <summary>
		/// Checks if this FileWriterNode instance has any child nodes.
		/// </summary>
		/// <returns>
		/// The function returns true if this FileWriterNode instance has at least one
		/// child node and false otherwise.
		/// </returns>
		bool HasChildNodes() const;

	private:
		std::vector<FileWriterNode> mChildNodes;
		std::string mOutputText;
	};
}
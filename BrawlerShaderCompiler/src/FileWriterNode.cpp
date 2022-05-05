module;
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <cassert>

module Brawler.FileWriterNode;

namespace Brawler
{
	void FileWriterNode::SetOutputText(std::string&& outputText)
	{
		assert(!HasChildNodes() && "ERROR: An attempt was made to give a FileWriterNode instance text to write out after it was already assigned child nodes!");

		mOutputText = std::move(outputText);
	}

	void FileWriterNode::AddChildNode(FileWriterNode&& childNode)
	{
		assert(mOutputText.empty() && "ERROR: An attempt was made to assign a child node to a FileWriterNode instance after it was already given text to write out!");

		mChildNodes.push_back(std::move(childNode));
	}

	void FileWriterNode::WriteOutputText(std::ofstream& fileStream) const
	{
		if (HasChildNodes())
		{
			for (const auto& childNode : mChildNodes)
				childNode.WriteOutputText(fileStream);
		}
		else
			fileStream << mOutputText;
	}

	bool FileWriterNode::HasChildNodes() const
	{
		return !(mChildNodes.empty());
	}
}
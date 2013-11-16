#pragma once
#include <string>
#include <ostream>

namespace huffman
{
	std::string compress(std::string);
	std::string decompress(std::string);

	void pretty_tree(std::ostream&, std::string);
}

#include "huffman.h"
#include <iostream>
#include <string>
#include <fstream>
#include <cassert>

std::string readFile(const std::string& fileName)
{
	std::string ret;
	char* memblock;
	std::ifstream::pos_type size;

	std::ifstream file(fileName, std::ios::in | std::ios::binary | std::ios::ate);
	if (file.is_open())
	{
		size = file.tellg();
		memblock = new char[size];
		file.seekg(0, std::ios::beg);
		file.read(memblock, size);
		file.close();
		ret = std::string(memblock, memblock + size);

		delete[] memblock;
	}

	return std::move(ret);
};

void saveToFile(const std::string& fileName, const std::string& content)
{
	std::string ret;
	char* memblock;
	std::ifstream::pos_type size;

	std::ofstream file(fileName, std::ios::out | std::ios::binary);
	if (file.is_open())
	{
		size = content.size();
		memblock = new char[size];

		memcpy_s(memblock, size, content.c_str(), size);

		file.seekp(0, std::ios::beg);
		file.write(memblock, size);
		file.close();
		ret = std::string(memblock, memblock + size);

		delete[] memblock;
	}

}

int main()
{
	std::string poem = readFile("poem.txt");
	std::string compressed = huffman::compress(poem);
	std::string decompressed = huffman::decompress(compressed);
	assert(poem == decompressed);

	std::string pedia = readFile("huffmanpedia.txt");
	compressed = huffman::compress(pedia);
	decompressed = huffman::decompress(compressed);
	assert(pedia == decompressed);
	huffman::pretty_tree(std::cout, "ett annat exempel");
}
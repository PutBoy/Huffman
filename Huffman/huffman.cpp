#include "huffman.h"

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <array>
#include <sstream>


namespace huffman
{
	namespace detail
	{
		class Node
		{
		public:
			Node()
			{}

			Node(char chr, int weight)
				:chr_(chr)
				,weight_(weight)
			{}

			Node(Node* lhs, Node* rhs)
				:lhs_(lhs->weight_ > rhs->weight_ ? lhs : rhs)
				, rhs_(lhs->weight_ > rhs->weight_ ? rhs : lhs)
				,weight_(lhs->weight_ + rhs_->weight_)
			{}

			void pretty(std::ostream& out);
			std::string encode();
			std::map<char, std::vector<bool>> make_map();
	
			int get_weight() const
			{
				return weight_;
			}

			char get_chr() const
			{
				return chr_;
			}
			
			void set_chr(char chr)
			{
				chr_ = chr;
			}

			Node* get_left() const
			{
				return lhs_.get();
			}
			Node* get_right() const
			{
				return rhs_.get();
			}

			void set_left(Node* other)
			{
				lhs_ = std::unique_ptr<Node>(other);
			}
			void set_right(Node* other)
			{
				rhs_ = std::unique_ptr<Node>(other);
			}
		private:
			void internal_pretty(std::ostream&, std::string&);
			void internal_encode(std::vector<bool>& bits, std::vector<bool>& bitsstring);
			void internal_make_map(std::map<char, std::vector<bool>>&, std::vector<bool>& bitsstring);

			char chr_;
			std::unique_ptr<Node> lhs_; 
			std::unique_ptr<Node> rhs_;
			int weight_;
		};

		template <class forward_iter>
		Node* make_tree(forward_iter begin, forward_iter end);

		template <class forward_iter>
		Node* decode_tree(forward_iter begin, forward_iter end);

		template <class forward_iter>
		std::string encode(Node*, forward_iter begin, forward_iter end);

		template <class forward_iter>
		std::string decode(Node*, forward_iter begin, forward_iter end);
		
	}
	using namespace detail;

	void pretty_tree(std::ostream& out, std::string str)
	{
		std::unique_ptr<Node> root(make_tree(str.begin(), str.end()));
		root->pretty(out);
	}

	std::string compress(std::string str)
	{
		std::string ret;
		std::unique_ptr<Node> root(make_tree(str.begin(), str.end()));
		if (root)
		{
			ret = root->encode();
			ret += encode(root.get(), str.begin(), str.end());
		}
		return ret;
	}

	std::string decompress(std::string str)
	{
		std::unique_ptr<Node> root(decode_tree(str.begin(), str.end()));
		return decode(root.get(), str.begin(), str.end());
	}
	
	namespace detail
	{
		template <class forward_iter>
		Node* make_tree(forward_iter begin, forward_iter end)
		{
			struct comp
			{
				bool operator()(const Node* lhs, const Node* rhs)
				{
					return lhs->get_weight() > rhs->get_weight();
				}
			};

			std::array<std::pair<char, int>, 256U> buckets;
			std::fill(buckets.begin(), buckets.end(), std::make_pair(0, 0));
			for (; begin != end; ++begin)
			{
				buckets[static_cast<unsigned char>(*begin)].first = *begin;
				buckets[static_cast<unsigned char>(*begin)].second++;
			}

			std::priority_queue<Node*, std::vector<Node*>, comp> queue;
			for (auto& bucket : buckets)
			{
				if (bucket.second != 0)
					queue.emplace(new Node(bucket.first, bucket.second));
			}
			
			while (!queue.empty())
			{
				auto node = queue.top();
				queue.pop();
				if (queue.empty())
					return node;
				auto other = queue.top();
				queue.pop();
				queue.emplace(new Node(node, other));
			}
			
			return nullptr;
		}

		void Node::pretty(std::ostream& out)
		{
			std::string bits;
			internal_pretty(out, bits);
		}

		void Node::internal_pretty(std::ostream& out, std::string& bits)
		{
			if (lhs_ == nullptr && rhs_ == nullptr)
			{
				out << chr_ << ':' << bits << std::endl;
			}
			if (lhs_)
			{
				bits.push_back('1');
				lhs_->internal_pretty(out, bits);
				bits.pop_back();
			}
			if (rhs_)
			{
				bits.push_back('0');
				rhs_->internal_pretty(out, bits);
				bits.pop_back();
			}
		}

		/* encoding of the tree given a string:
			the first 4 bytes is, in big endian,
			the length of the encoded string 
			in bits */
		template <class forward_iter>
		std::string encode(Node* root, forward_iter begin, forward_iter end)
		{
			std::vector<bool> bits;
			std::string ret;
			auto code = root->make_map();
			for (; begin != end; begin++)
			{
				for (auto bit : code[*begin])
				{
					bits.push_back(bit);
				}
			}

			int i = 0;
			unsigned char thebit = 0;
			for (auto bit : bits)
			{
				thebit |= (bit << (7 - (i % 8)));
				++i;
				if (i % 8 == 0)
				{
					ret.push_back(thebit);
					thebit = 0;
				}
			}
			if (i % 8 != 0)
				ret.push_back(thebit);

			unsigned int bytes = bits.size();
			unsigned char bytes1 = bytes & 255;
			unsigned char bytes2 = (bytes >> 8);
			unsigned char bytes3 = (bytes >> 16);
			unsigned char bytes4 = (bytes >> 24);
			ret.insert(ret.begin(), bytes4);
			ret.insert(ret.begin() + 1, bytes3);
			ret.insert(ret.begin() + 2, bytes2);
			ret.insert(ret.begin() + 3, bytes1);

			return ret;

		}

		/* encoding of the tree:
		Big Endian
		First 2 bytes is the length of the tree encoding (in bytes).
		Next every node of the tree is encoding in this pattern:
		8bits, length of the bit string.
		bit string (with as many bits as indicated earlier
		character, in 8 bits.
		*/
		std::string Node::encode()
		{
			std::string ret;
			std::vector<bool> bits;
			std::vector<bool> bitstring;
			internal_encode(bits, bitstring);
			unsigned short bytes = bits.size();
			unsigned char bytes1 = bytes & 255;
			unsigned char bytes2 = (bytes >> 8);
			ret.push_back(bytes2);
			ret.push_back(bytes1);

			int i = 0;
			unsigned char thebit = 0;
			for (auto bit : bits)
			{
				thebit |= (bit << (7 - (i % 8)));
				++i;
				if (i % 8 == 0 && i != 0)
				{
					ret.push_back(thebit);
					thebit = 0;
				}
			}
			if (i % 8 != 0)
				ret.push_back(thebit);

			return ret;
		}

		void Node::internal_encode(std::vector<bool>& bits, std::vector<bool>& bitstring)
		{
			if (lhs_ == nullptr && rhs_ == nullptr)
			{
				size_t size = bitstring.size();
				for (int i = 1; i <= 8; ++i)
				{
					bits.push_back(size >> (8 - i) & 1);
				}
				for (auto bit : bitstring)
				{
					bits.push_back(bit);
				}
				for (int i = 1; i <= 8; ++i)
				{
					bits.push_back(static_cast<unsigned char>(chr_) >> (8 - i) & 1);
				}
			};
			if (lhs_)
			{
				auto lbits = bitstring;
				lbits.push_back(true);
				lhs_->internal_encode(bits, lbits);
			}
			if (rhs_)
			{
				auto rbits = bitstring;
				rbits.push_back(false);
				rhs_->internal_encode(bits, rbits);
			}

		}

		std::map<char, std::vector<bool>> Node::make_map()
		{
			std::map<char, std::vector<bool>> ret;
			std::vector<bool> bitstring;
			internal_make_map(ret, bitstring);
			return ret;
		}
		
		void Node::internal_make_map(std::map<char, std::vector<bool>>& map, std::vector<bool>& bitsstring)
		{
			if (lhs_ == nullptr && rhs_ == nullptr)
			{
				map.insert(std::make_pair(chr_, bitsstring));
			}
			if (lhs_)
			{
				auto lbits = bitsstring;
				lbits.push_back(true);
				lhs_->internal_make_map(map, lbits);
			}
			if (rhs_)
			{
				auto rbits = bitsstring;
				rbits.push_back(false);
				rhs_->internal_make_map(map, rbits);
			}
		}

		template <class forward_iter>
		Node* decode_tree(forward_iter begin, forward_iter end)
		{
			int size = 0;
			if (std::distance(begin, end) >= 2)
			{
				unsigned short byte1 = static_cast<unsigned char>(*begin);
				unsigned short byte2 = static_cast<unsigned char>(*(++begin));
				byte1 <<= 8;
				size = byte1 + byte2;
				++begin;
			}
			
			Node* root = new Node();
			Node* current = root;
			for (int bit = 0; bit < size;)
			{
				auto bitat = [begin](int at) -> bool
					{
						auto byte = *(begin + at / 8);
						int bit = 7 - (at % 8);
						unsigned char chr = *(begin + at / 8) >> (7 - (at % 8));
						return (*(begin + at / 8) >> (7 - (at % 8)) & 1) == 1;
					};
				int len = 0;
				for (int i = 1; i <= 8; ++i)
				{
					len |= bitat(bit) << (8 - i);
					++bit;
				}

				for (int i = 0; i < len; ++i)
				{
					if (bitat(bit))
					{
						if (!current->get_left())
							current->set_left(new Node());
						current = current->get_left();
					}
					else
					{
						if (!current->get_right())
							current->set_right(new Node());
						current = current->get_right();
					}
					++bit;
				}

				unsigned char chr = 0;
				for (int i = 1; i <= 8; ++i)
				{
					bool b = bitat(bit);
					chr |= b << (8 - i);
					++bit;
				}

				current->set_chr(chr);
				current = root;
			}
			return root;
		}

		template <class forward_iter>
		std::string decode(Node* root, forward_iter begin, forward_iter end)
		{
			std::string ret;
			int treesize = 0;
			if (std::distance(begin, end) >= 2)
			{
				unsigned short byte1 = static_cast<unsigned char>(*begin);
				unsigned short byte2 = static_cast<unsigned char>(*(++begin));
				byte1 <<= 8;
				treesize = byte1 + byte2;
				++begin;
			}

			treesize /= 8;
			if (std::distance(begin, end) >= treesize)
				begin += treesize + 1;

			int datasize;
			if (std::distance(begin, end) >= 2)
			{
				unsigned int byte1 = static_cast<unsigned char>(*(begin));
				unsigned int byte2 = static_cast<unsigned char>(*(++begin));
				unsigned int byte3 = static_cast<unsigned char>(*(++begin));
				unsigned int byte4 = static_cast<unsigned char>(*(++begin));
				byte1 <<= 24;
				byte2 <<= 16;
				byte3 <<= 8;
				datasize = byte1 + byte2 + byte3 + byte4;
				++begin;
			}

			auto bitat = [begin](int at)
			{
				auto byte = *(begin + at / 8);
				int bit = 7 - (at % 8);
				unsigned char chr = *(begin + at / 8) >> (7 - (at % 8));
				return *(begin + at / 8) >> (7 - (at % 8)) & 1;
			};

			Node* current = root;
			for (int bit = 0; bit < datasize; ++bit)
			{
				if (!current->get_left() && !current->get_right())
				{
					ret.push_back(current->get_chr());
					current = root;
				}

				if (bitat(bit))
				{
					current = current->get_left();	
				}
				else
				{
					current = current->get_right();
				}
			}

			if (!current->get_left() && !current->get_right())
			{
				ret.push_back(current->get_chr());
				current = root;
			}
			return ret;
		}

	}
}
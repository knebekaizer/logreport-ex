/// @file

#ifndef IPLOG_TEST_TRIE_H
#define IPLOG_TEST_TRIE_H

#include "subnets.h"
#include "err_policy.h"
#include "trace.h"

#include <cstdint>
#include <iostream>

#ifdef IPLOG_SELFTEST
#include <vector>
#endif



/**
@Invariants:
 + begin < end (eq for exact dup)
 + end <= size
 + subs[k].begin == this->end Do I need to store it twice? Traverse over the chain always
*/

class Payload;



/// @todo Use intrinsic for HW support
inline uint8_t leftmostbit(uint32_t x)
{
	int32_t y = x; // use sign bit
	uint8_t b = 0;
	for ( ; b < 32; ++b) {
		if ((y << b) < 0) break;
	}
	return b;
}

/// @returns Position of first different bit
inline uint8_t diffbit(uint32_t a1, uint32_t a2)
{
	int32_t y = a1 ^ a2; // use sign bit
	uint8_t b = 0;
	for ( ; b < 32; ++b) {
		if ((y << b) < 0) break;
	}
	return b;
}

namespace trie {

template <class IP>
struct Node {

	Node() = default;

	Node(const IP& i, int b, int e)
		: ip(i)
		, begin(b)
		, end(e)
		, subs{0,0}
	{
	}

    int setChild(Node* node);

	int size() const { return ip.size(); }
	typename IP::AddrT addr() const { return ip.addr(); }

//	const IP* const ip;  // chain members share the same pointer to save memory
    IP ip;         // sizeof = 8 or 24 for the cost of indirection. Tune for cache line size
    uint8_t begin = 0;   // [begin, end) as a network bits, eg [0, 128)
    uint8_t end = 8 * sizeof(typename IP::AddrT);
    Node *subs[2] = {0,0};    // struct { Node *zero, *one; } subs;

	Payload* data = nullptr;
};
// Should I improve padding by inlining IP into Node? Or use attr packed

using Node4 = Node<IP>;

template <class IP>
class Radix {
public:
    Node<IP>* insert(const IP& x);
    Node<IP>* lookup(const IP& x);

    Node<IP> root;


#ifdef IPLOG_SELFTEST
	void selfTest_add(const std::string& ip) { all.emplace_back(ip); }
	void selfTest_run();

	mutable std::vector<std::string> all;
#else
	#define selfTest_add(...)
	#define selfTest_run()
#endif

};

template<typename IP>
trie::Node<IP>* Radix<IP>::insert(const IP& x)
{
	auto c = &root;  // current node, start from root
	Node4* parent = nullptr;
	while (1) {
		assert(c != nullptr); // better than while(c) as it makes assumption clearer
		auto prefix = std::min(c->end, std::min(x.size(),
		                                        diffbit(c->addr(), x.addr())));  // in [ c.begin, min(c.end, x.size) )
		assert(prefix > c->begin || c == &root); // for any node but root. Root may have zero prefix

		if (prefix == x.size()) {
			assert(x.size() < c->size());  // otherwise prefix can't be eq x.size
			if (x.size() == c->end) {
				// Don't create empty split; c->end > c->size so there is a tail somewhere - use it
				// just replace IP data
				c->ip = x;
				return c;
			}
			// insert x right before node
			auto n = new Node4(x, c->begin, prefix);
			c->begin = prefix;
			n->setChild(c);
			assert(parent != nullptr);
			parent->setChild(n);
			return n; // done
		}

		// else insert after
		if (prefix < c->end) {
			//  Split at prefix position;
			auto n0 = new Node4(*c);  //  = node[pos:] May remove unused bytes from IP but should i care?
			c->end = prefix;         //  = node[0:pos] Truncate
			n0->begin = prefix;
			c->setChild(n0);         // place tail into chain

			auto n = new Node4(x, prefix, x.size());

			// Let's check that __both__ children actually updated
			assert(bit(n0->addr(), n0->begin) != bit(n->addr(), n->begin));

			c->setChild(n);        // Both children are updated in place
			return n;
		}

		assert(prefix == c->end);
		Node4** next = &c->subs[ bit(x.addr(), prefix) ];
		if (*next == nullptr) {
			*next = new Node4(x, prefix, x.size());
			return *next;
		} else {
			// else dive deeper
			parent = c;
			c = *next;
		}
	}
}


template<typename IP>
trie::Node<IP>* Radix<IP>::lookup(const IP &x)
{
	trie::Node4* c = &root;  // current node, start from root
	trie::Node4* best = &root;
	while (1) {
//Trace2(*c, x);
		assert(c != nullptr); // better than while(c) as it makes assumption clearer

		auto prefix = std::min(c->end, std::min(x.size(),
		                                        diffbit(c->addr(), x.addr())));  // in [ c.begin, min(c.end, x.size) )

//Trace2((int)prefix, (int)c->end);
		if (prefix == x.size()) {
			if (c->size() == c->end) {
				best = c;
			}
			return best;
		} else if (prefix < c->end)
			return best;

		assert(prefix == c->end);
		if (c->size() == c->end) {
			best = c;
		}

//Trace2(x, bit(x.addr, c->end));
		c = c->subs[bit(x.addr(), c->end)];
		if (!c) {
			return best; // found
		}
	}
}


template<typename IP>
int Node<IP>::setChild(Node<IP>* node)
{
	assert(node);
	assert(node->begin >= this->end);
	unsigned int k = bit(node->addr(), node->begin);
	assert(k <= 1);
	subs[k] = node; // odd or even
	return k;
}

#ifdef IPLOG_SELFTEST
template <typename IP>
void trie::Radix<IP>::selfTest_run()
{
	for (auto const& x : all) {
		IP ip(x);
		auto f = lookup(ip);
		require(f->ip == ip) << " with f = " << *f << "; x = " << ip;
		assert(f->ip == ip);
	}
	log_info << "[Trie::SelfTest] Trie integrity check passed";
}
#endif

} // namespace trie;


void outline(trie::Node4* p, int level = 0);


inline
std::ostream& operator<<(std::ostream& os, const IP& ip)
{
	auto a = ip.addr();
    os << (a>>24) << '.' << ((a>>16) & 0xff) << '.' << ((a>>8) & 0xff) << '.' << (a & 0xff) << '/' << (int)ip.size();
    return os;
}

std::ostream& operator<<(std::ostream& os, const trie::Node4& ip);
std::ostream& operator<<(std::ostream& os, const trie::Radix<IP>& trie);



#endif //IPLOG_TEST_TRIE_H

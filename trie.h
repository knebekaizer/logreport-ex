/// @file

#ifndef IPLOG_TEST_TRIE_H
#define IPLOG_TEST_TRIE_H

#include "subnets.h"
#include "err_policy.h"
#include "trace.h"

#include <cstdint>
#include <iostream>
#include <deque>

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
		if (int32_t(y << b) < 0) break;
	}
	return b;
}

/// @return 0 or 1 only (nefer 0b100).
inline uint8_t diffbit(IPv6::AddrT a1, IPv6::AddrT a2)
{
	for (int i=0; i<sizeof(IPv6::AddrT); ++i) {
		int8_t y = a1[i] ^ a2[i];
		if (y) {
			for (uint8_t b = 0; b < 8; ++b) {
				if (int8_t(y << b) < 0)
					return i * 8 + b;
			}
		}
	}
	return IPv6::addr_size;
}

namespace trie {

template <class IP>
struct Node {

	Node() = default;

	Node(const IP& i, int b, int e, Payload* p)
		: ip(i)
		, begin(b)
		, end(e)
		, subs{0,0}
		, data(p)
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


// this is quick-n-dirty prototype of fast linear allocator (stack allocator)
// Node is a POD and never deallocated, so simple page-based memory pool
// would be OK.
template <class IP>
class PoolAlloc {
public:
	trie::Node<IP>* createNode(const IP& i, int b, int e, Payload* p) {
		buf.emplace_back(i, b, e, p);
		return &buf.back();
	}
	trie::Node<IP>* createNode(const trie::Node<IP>& c) {
		buf.push_back(c);
		return &buf.back();
	}
private:
	std::deque<typename trie::Node<IP>> buf;
};


template <class IP>
class Radix : PoolAlloc<IP> {
public:
    Node<IP>* insert(const IP& x, Payload* payload);
    Node<IP>* lookup(const IP& x);
    using PoolAlloc<IP>::createNode;

    Node<IP> root;

public:
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
trie::Node<IP>* Radix<IP>::insert(const IP& x, Payload* payload)
{
	auto c = &root;  // current node, start from root
	Node<IP>* parent = nullptr;
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
				c->data = payload;
				return c;
			}
			// insert x right before node
			auto n = createNode(x, c->begin, prefix, payload);
			c->begin = prefix;
			n->setChild(c);
			assert(parent != nullptr);
			parent->setChild(n);
			return n; // done
		}

		// else insert after
		if (prefix < c->end) {
			//  Split at prefix position;
			auto n0 = createNode(*c);  //  = node[pos:] May remove unused bytes from IP but should i care?
			c->end = prefix;         //  = node[0:pos] Truncate
			n0->begin = prefix;
			c->setChild(n0);         // place tail into chain

			auto n = createNode(x, prefix, x.size(), payload);

			// Let's check that __both__ children actually updated
			assert(bit(n0->addr(), n0->begin) != bit(n->addr(), n->begin));

			c->setChild(n);        // Both children are updated in place
			return n;
		}

		assert(prefix == c->end);
		Node<IP>** next = &c->subs[ bit(x.addr(), prefix) ];
		if (*next == nullptr) {
			*next = createNode(x, prefix, x.size(), payload);
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
	trie::Node<IP>* c = &root;  // current node, start from root
	trie::Node<IP>* best = &root;
	while (1) {
		assert(c != nullptr); // better than while(c) as it makes assumption clearer

		auto prefix = std::min(c->end, std::min(x.size(),
		                                        diffbit(c->addr(), x.addr())));  // in [ c.begin, min(c.end, x.size) )

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


void outline(trie::Node<IP>* p, int level = 0);

void walk(const trie::Node<IP>* p, int level = 0);
void walk(const trie::Node<IPv6>* p, int level = 0);

template<typename IP>
std::ostream& operator<<(std::ostream& os, const trie::Radix<IP>& trie) {
//	walk(&trie.root, 0, [](auto p, int level){ std::cout << std::string(level, '\t') << *p << std::endl; });
	walk(&trie.root);
	return os;
}


template<typename IP>
std::ostream& operator<<(std::ostream& os, const trie::Node<IP>& n)
{
	os << n.ip << "-" << (int)n.begin << ":" << (int)n.end;
	return os;
}



#endif //IPLOG_TEST_TRIE_H

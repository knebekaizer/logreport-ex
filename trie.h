/// @file

#ifndef IPLOG_TEST_TRIE_H
#define IPLOG_TEST_TRIE_H

#include "subnets.h"

#include <cstdint>
#include <iostream>

#ifdef IPLOG_SELFTEST
#include <vector>
#endif


namespace trie {
struct Node;
}


/**
@Invariants:
 + begin < end (eq for exact dup)
 + end <= size
 + subs[k].begin == this->end Do I need to store it twice? Traverse over the chain always
*/

class Payload;

namespace trie {

struct Node {

	Node() = default;

    Node(const IP& ip, int begin, int pos);

    int setChild(Node* node);

	int size() const { return ip.size(); }
	IP::AddrT addr() const { return ip.addr(); }

//	const IP* const ip;  // chain members share the same pointer to save memory
    IP ip;         // sizeof = 8 or 24 for the cost of indirection. Tune for cache line size
    uint8_t begin = 0;   // [begin, end) as a network bits, eg [0, 128)
    uint8_t end = 8 * sizeof(IP::AddrT);
    Node *subs[2] = {0,0};    // struct { Node *zero, *one; } subs;

	Payload* data = nullptr;
};
// Should I improve padding by inlining IP into Node? Or use attr packed


class Radix {
public:
    Node* insert(const IP& x);
    Node* lookup(const IP& x);

    Node root;


#ifdef IPLOG_SELFTEST
	void selfTest_add(const std::string& ip) { all.emplace_back(ip); }
	void selfTest_run();

	mutable std::vector<std::string> all;
#else
	#define selfTest_add(...)
	#define selfTest_run()
#endif

};

} // namespace trie;


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


void outline(trie::Node* p, int level = 0);


inline
std::ostream& operator<<(std::ostream& os, const IP& ip)
{
	auto a = ip.addr();
    os << (a>>24) << '.' << ((a>>16) & 0xff) << '.' << ((a>>8) & 0xff) << '.' << (a & 0xff) << '/' << (int)ip.size();
    return os;
}

std::ostream& operator<<(std::ostream& os, const trie::Node& ip);
std::ostream& operator<<(std::ostream& os, const trie::Radix& trie);


#endif //IPLOG_TEST_TRIE_H

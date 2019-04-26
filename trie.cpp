/// @file

#include "trie.h"

#include <cstdint>
#include <cassert>
#include <algorithm>

/**
 * Immutable object
 */
struct IP {
    using AddrT = uint32_t;
    using MaskT = uint8_t;
	const AddrT addr;
	const MaskT bits;  ///< bitmask length, number of network bits
	uint8_t size() const { return bits; }
};

/**
@Invariants:
 + begin < end (eq for exact dup)
 + end <= size
 + subs[k].begin == this->end Do I need to store it twice? Traverse over the chain always
*/

namespace trie {

struct Node {

    Node(const Node& node, int begin, int end);
    Node(const IP& ip, int begin, int pos);

    int setChild(Node* node);

//	const IP* const ip;  // chain members share the same pointer to save memory
    const IP ip;         // sizeof = 8 or 24 for the cost of indirection. Tune for cache line size
    uint8_t begin, end;  // [begin, end) as a network bits, eg [0, 128)
    Node *subs[2];    // struct { Node *zero, *one; } subs;

    int size() const { return ip.size(); }
    IP::AddrT addr() const { return ip.addr; }
};
// Should I improve padding by inlining IP into Node? Or use attr packed


class Radix {
public:
    void insert(const IP& x);

    Node root;
};

} // namespace trie;

using namespace trie;
using trie::Node;

/// @return 0 or 1 only (nefer 0b100).
inline unsigned int bit(IP::AddrT x, unsigned int bit)
{
	constexpr int sz = 8 * sizeof(x) - 1;
	assert(bit <= sz);
	return 1 & (x >> (sz - bit));
}



//IP x
//insert x
void Radix::insert(const IP& x)
{
    auto c = &root;  // current node, start from root
    Node* parent = nullptr;
    while (1) {
        assert(c != nullptr); // better than while(c) as it makes assumption clearer
        auto prefix_end = std::min(c->end, std::min(x.size(), leftmostbit(c->addr() ^ x.addr)));  // in [ c.begin, min(c.end, x.size) )
        assert(prefix_end > c->begin || c == &root); // for any node but root. Root may have zero prefix

        if (prefix_end == x.size()) {
            assert(x.size() < c->size());  // otherwise prefix can't be eq x.size
            // insert x right before node
            auto n = new Node(x, c->begin, prefix_end);
            n->setChild(c);
            assert(parent != nullptr);
            parent->setChild(n);
            break; // done
        }

        // else insert after
        if (prefix_end < c->end) {
        //  c = split(c, prefix_end);
            auto n0 = new Node(c->ip, c->begin, prefix_end);  //  = node[0:pos] Make shallow copy and truncate
		    c->begin = prefix_end;             // = node[pos:] May remove unused bytes from IP but should i care?
		    n0->setChild(c);           // place tail into chain
		    if (parent != nullptr) {
			    parent->setChild(n0);          // replace node by n0 in the parent to fix chain
		    } else {
		    	assert(c == &root);
		    }
		    c = n0;                       // and set current to the new one

            // if i had to split then postcondition is: single child that no common prefix with x
            assert(prefix_end == c->end); // after splitting
            c->setChild(new Node(x, prefix_end, x.size())); // NB: c has been changed!
            break; // done
        }

        assert(prefix_end == c->end);
        Node** next = &c->subs[ bit(c->addr(), c->begin) ];
        if (*next == nullptr) {
            *next = new Node(x, prefix_end, x.size());
            break;
        }

        // else dive deeper
        parent = c;
        c = *next;
    }
}

int Node::setChild(Node* node)
{
    unsigned int k = bit(node->addr(), node->begin);
    assert(k <= 1);
    subs[k] = node; // odd or even
    return k;
}

/*
Node* split(Node& node, int pos)
{
    auto n0 = new Node(node.ip, node.begin, pos);    //  = node[0:pos] Make shallow copy and truncate
    node.begin = pos;                 // = node[pos:] May remove unused bytes from IP but should i care?
    n0->setChild(node);                // place tail into chain
    parent.setChild(n0);    // replace node by n0 in the parent to fix chain
    return n0;
}
*/
/// @file

#include "trie.h"

#include <cstdint>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <vector>

#include "trace.h"

using namespace trie;

trie::Node::Node(const IP& i, int b, int e)
	: ip(i)
	, begin(b)
	, end(e)
	, subs{0,0}
{
}


/// @return 0 or 1 only (nefer 0b100).
inline unsigned int bit(IP::AddrT x, unsigned int bit)
{
	constexpr int sz = 8 * sizeof(x) - 1;
	if (!(bit <= sz)) log_error << "!(bit <= sz) " << bit;
	assert(bit <= sz);
	return 1 & (x >> (sz - bit));
}

#undef DEF_LOG_LEVEL
#define DEF_LOG_LEVEL (LOG_LEVEL::info)

//IP x
//insert x
void Radix::insert(const IP& x)
{
    auto c = &root;  // current node, start from root
    Node* parent = nullptr;
    while (1) {
//outline(&root);
        assert(c != nullptr); // better than while(c) as it makes assumption clearer
        auto prefix = std::min(c->end, std::min(x.size(), leftmostbit(c->addr() ^ x.addr)));  // in [ c.begin, min(c.end, x.size) )
Trace2(*c, x);
TraceX((int)prefix);
        assert(prefix > c->begin || c == &root); // for any node but root. Root may have zero prefix

        if (prefix == x.size()) {
            assert(x.size() < c->size());  // otherwise prefix can't be eq x.size
        	if (x.size() == c->end) {
        		// Don't create empty split; c->end > c->size so there is a tail somewhere - use it
        		// just replace IP data
        		c->ip = x;
Trace2(01, *c);
        		break;
	        }
            // insert x right before node
            auto n = new Node(x, c->begin, prefix);
            c->begin = prefix;
            n->setChild(c);
            assert(parent != nullptr);
            parent->setChild(n);
Trace2(1, *n);
            break; // done
        }

        // else insert after
        if (prefix < c->end) {
 //       	if (c != &root) {
		        //  c = split(c, prefix);
		        auto n0 = new Node(*c);  //  = node[0:pos] Make shallow copy and truncate
		        c->end = prefix;             // = node[pos:] May remove unused bytes from IP but should i care?
		        n0->begin = prefix;
		        c->subs[0] = 0;
		        c->subs[1] = 0;
		        c->setChild(n0);           // place tail into chain

		        // if i had to split then postcondition is: single child that no common prefix with x
//	        } else {
//		        c->end = prefix;
//	        }
            assert(prefix == c->end); // after splitting
            auto n = new Node(x, prefix, x.size());
            c->setChild(n); // NB: c has been changed!
Trace2(2, *n);
            break; // done
        }

        assert(prefix == c->end);
        Node** next = &c->subs[ bit(x.addr, prefix) ];
Trace2(x, bit(x.addr, prefix));
        if (*next == nullptr) {
            *next = new Node(x, prefix, x.size());
Trace2(3, **next);
            break;
        }

        // else dive deeper
        parent = c;
        c = *next;
Trace2("4 next", *c);
    }

#ifndef NDEBUG
	checkInvariants(x);
    // check invariant
    auto f = lookup(x);
    assert(f->addr() == x.addr && f->size() == x.size());
#endif
}

namespace trie {

Node* Radix::lookup(const IP &x)
{
	auto c = &root;  // current node, start from root
	while (1) {
//TraceX(*c);
		assert(c != nullptr); // better than while(c) as it makes assumption clearer
		auto prefix = std::min(c->end, std::min(x.size(), leftmostbit(
				c->addr() ^ x.addr)));  // in [ c.begin, min(c.end, x.size) )

//Trace2((int)prefix, (int)c->end);
		if (prefix < c->end || prefix == x.size())
			return c;
//Trace2(x, bit(x.addr, c->end));
		auto p = c->subs[bit(x.addr, c->end)];
		if (p) {
			assert(p->begin >= c->end);
			c = p;
		} else {
			return c; // found
		}

	}
}


#ifdef IPLOG_DEBUG
void Radix::checkInvariants(const IP& ip)
{
	static std::vector<IP> all;;
	all.push_back(ip);
	for (auto const& x : all) {
	    auto f = lookup(x);
	    bool cond = (f->addr() == x.addr && f->size() == x.size());
	    if (!cond) {
		    log_error << "Failed condition (f->addr() == x.addr && f->size() == x.size()) with "
		                 "f = " << *f << "; x = " << x;
		//    outline(&root);
	    }

	    assert(f->addr() == x.addr && f->size() == x.size());
	}
}
#endif

} // namespace trie

int trie::Node::setChild(trie::Node* node)
{
	assert(node);
Trace2(*this, *node);
	assert(node->begin >= this->end);
    unsigned int k = bit(node->addr(), node->begin);
    assert(k <= 1);
    subs[k] = node; // odd or even
    return k;
}


void outline(trie::Node* p, int level)
{
    if (!p) {
        std::cout << std::string(level, '\t') << 0 << std::endl;
        return;
    }

    std::cout << std::string(level, '\t') << *p << std::endl;
    outline(p->subs[0], level+1);
    outline(p->subs[1], level+1);
}

void walk(trie::Node* p, int level)
{
    if (!p) {
//        std::cout << std::string(level, '\t') << 0 << std::endl;
        return;
    }

    if (p->size() == p->end) {
	    std::cout << std::string(level, '\t') << *p << std::endl;
	    ++level;
    }

    if (p->subs[0] && p->subs[1] && p->subs[0]->addr() < p->subs[1]->addr()) {
	    walk(p->subs[0], level);
	    walk(p->subs[1], level);
	} else {
	    walk(p->subs[1], level);
	    walk(p->subs[0], level);
    }
}


std::ostream& operator<<(std::ostream& os, const trie::Node& n)
{
    os << n.ip << "-" << (int)n.begin << ":" << (int)n.end;
    return os;
}


#ifdef UT_CATCH
#include "catch.hpp"


TEST_CASE( "bit", "[bit]")
{
	//             01234567890123456789012345678901
    REQUIRE( bit(0b11111111111101111111111111111111, 12) == 0 ) ;
    REQUIRE( bit(0b00000000000000000000000000000001, 31) == 1 ) ;
    REQUIRE( bit(0b10000000000000000000000000000000, 0) == 1 ) ;
    REQUIRE( bit(0b01111111111111111111111111111111, 0) == 0 ) ;
    REQUIRE( bit(0b11111111111111111111111111111111, 5) == 1 ) ;
}
#endif // UT_CATCH

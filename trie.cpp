/// @file

#include "trie.h"

#include <cstdint>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <vector>

#include "trace.h"
#include "err_policy.h"

using namespace trie;



void outline(trie::Node<IP>* p, int level)
{
    if (!p) {
        std::cout << std::string(level, '\t') << 0 << std::endl;
        return;
    }

    std::cout << std::string(level, '\t') << *p << std::endl;
    outline(p->subs[0], level+1);
    outline(p->subs[1], level+1);
}


template<class IP, class function>
void walk(const trie::Node<IP>* p, int level, function f)
{
    if (!p) {
        return;
    }

    if (p->size() == p->end) {
    	f(p, level);
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

void walk(const trie::Node<IP>* p, int level) {
	walk(p, level, [](auto p, int level){ std::cout << std::string(level, '\t') << *p << std::endl; });
}

void walk(const trie::Node<IPv6>* p, int level) {
	walk(p, level, [](auto p, int level){ std::cout << std::string(level, '\t') << *p << std::endl; });
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

    IPv6::AddrT a; std::fill(a.begin(), a.end(), 1); a[9] = 0b11101111;
    REQUIRE( bit(a, 68) == 0 );

	std::fill(a.begin(), a.end(), 0); a[0] = 0b10000000;
	REQUIRE( bit(a, 0) == 1 );
	REQUIRE( bit(a, 12) == 0 );
	REQUIRE( bit(a, 127) == 0 );
	a[15] = 1;
	REQUIRE( bit(a, 127) == 1 );

}
#endif // UT_CATCH

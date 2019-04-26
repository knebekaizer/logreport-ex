//
// Created by Vladimir Ivanov on 2019-04-26.
//

#ifndef IPLOG_TEST_TRIE_H
#define IPLOG_TEST_TRIE_H

#endif //IPLOG_TEST_TRIE_H

#include <cstdint>


inline uint8_t leftmostbit(uint32_t x)
{
	int32_t y = x; // use sign bit
	uint8_t b = 0;
    for ( ; b < 32; ++b) {
    	if ((y << b) < 0) break;
    }
    return b;
}

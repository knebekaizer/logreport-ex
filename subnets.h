/// @file

#ifndef IPLOG_TEST_SUBNETS_H
#define IPLOG_TEST_SUBNETS_H

#include <cstdint>
#include <set>

#include <iostream>
#include <string>

#include <arpa/inet.h>

#include "trace.h"


struct IP4Address {
    IP4Address() {}
    IP4Address(const std::string& addr);

    uint32_t    iaddr = 0; // host endian
};

struct IP4Network : public IP4Address{
    IP4Network() {}
    IP4Network(const std::string& addr, int b);

    bool supernetOf(const IP4Address& IP4Network) const;
    bool supernetOf(const IP4Network& IP4Network) const;

//    uint32_t    iaddr = 0; // host endian
    uint8_t     bits = 0;
//private:
//    in_addr_t   addr = 0;
};

inline
std::ostream& operator<<(std::ostream& os, const IP4Network& ip)
{
    os << (ip.iaddr>>24) << '.' << ((ip.iaddr>>16) & 0xff) << '.' << ((ip.iaddr>>8) & 0xff) << '.' << (ip.iaddr & 0xff) << '/' << (int)ip.bits;
    return os;
}


struct IPData : public IP4Network {
    IPData() : IP4Network() {}
    IPData(const std::string& ip, int b) : IP4Network(ip, b) {}

    void incr(uint64_t x) { data_ += x; }
    uint64_t data_ = 0;
};


class Node : public IPData {
public:
    Node() : IPData() {}
    Node(uint32_t addr) { iaddr = addr; bits = 32; }
    Node(const std::string& ip, int b) : IPData(ip, b) {}
    friend class Tree;

//private:
//  CountT  data_ = 0;
    struct less : std::binary_function<Node*, Node*, bool> {
        bool operator()(const Node* x, const Node* y) const; // { return x->ip->iaddr < y->ip->iaddr; }
    };
    std::set<Node*, Node::less>  subs;
};

class Tree : public Node {
public:
    Tree() : Node() {} // @todo fix leak
    void insert(Node* node);
    Node* lookup(const IP4Address&);
};


std::ostream& operator<<(std::ostream& os, const Tree& tree);

/*
class RNode : public IP4Network
{
public:
    RNode() : IP4Network() {}
    RNode(const IP4Network ip, uint8_t len) : IP4Network(ip), plen(len) {}
    uint8_t plen = 0;
    RNode* left = 0;
    RNode* right = 0;
};


class Radix : public RNode
{
public:
//    RNode* lookup(const IP4Network& ip) const;
    void insert(const IP4Network& ip) {};
};

void walk(RNode* p, int level = 0);
void outline(RNode* p, int level = 0);

inline
std::ostream& operator<<(std::ostream& os, const RNode& ip)
{
    os <<
    static_cast<const IP4Network&>(ip)
    << " - " << (int)ip.plen;
    return os;
}
*/
/*
inline unsigned int leftmostbit(uint32_t x)
{
	int32_t y = x; // use sign bit
	unsigned int b = 0;
    for ( ; b < 32; ++b) {
    	if ((y << b) < 0) break;
    }
    return b;
}
*/

#endif //IPLOG_TEST_SUBNETS_H

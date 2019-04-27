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

    uint32_t    addr = 0; // host endian
};

struct IP4Network : public IP4Address
{
    IP4Network() {}
    IP4Network(const std::string& addr, int b);

    bool supernetOf(const IP4Address& IP4Network) const;
    bool supernetOf(const IP4Network& IP4Network) const;

    uint8_t     bits = 0;
};


struct IP {
	IP() = default;
	explicit IP(std::string s);  // s by value
	explicit IP(std::string s, int bits);
	explicit IP(const IP4Network& ip) : addr(ip.addr), bits(ip.bits) {}

	using AddrT = uint32_t;
	using MaskT = uint8_t;
	static constexpr auto addr_size = 32;

	AddrT addr = 0;
	MaskT bits = addr_size;  ///< network mask length
	uint8_t size() const { return bits; }

	bool operator==(const IP& other) const {
		return addr == other.addr && bits == other.bits;
	}
private:
	void parse(const std::string& s);
};


inline
std::ostream& operator<<(std::ostream& os, const IP4Network& ip)
{
    os << (ip.addr>>24) << '.' << ((ip.addr>>16) & 0xff) << '.'
            << ((ip.addr>>8) & 0xff) << '.' << (ip.addr & 0xff) << '/' << (int)ip.bits;
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
    explicit Node(uint32_t a) { addr = a; bits = 32; }
    Node(const std::string& ip, int b) : IPData(ip, b) {}
    friend class Tree;

//private:
//  CountT  data_ = 0;
    struct less : std::binary_function<Node*, Node*, bool> {
        bool operator()(const Node* x, const Node* y) const; // { return x->ip->addr < y->ip->addr; }
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

#endif //IPLOG_TEST_SUBNETS_H

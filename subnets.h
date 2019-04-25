/// @file

#ifndef IPLOG_TEST_SUBNETS_H
#define IPLOG_TEST_SUBNETS_H

#endif //IPLOG_TEST_SUBNETS_H

#include <cstdint>
#include <set>

#include <iostream>
#include <string>

#include <arpa/inet.h>

#include "trace.h"

class IP {
public:
    using AddrT = uint64_t;
    using MaskT = uint64_t;
    AddrT   addr() const { return addr_; }
    MaskT   mask() const { return mask_; }
private:
    AddrT   addr_ = 0;
    MaskT   mask_ = ~0;
};

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
//    auto a = (const uint8_t*)&ip.addr;
//    os << (int)a[0] << '.' << (int)a[1] << '.' << (int)a[2] << '.' << (int)a[3] << '/' << (int)ip.bits;
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
class Node : public IPData {
public:
    friend class Tree;
//    using CountT = uint64_t; // need more bits?
//    CountT  data() const { return data_; }
//    CountT  addData(uint64_t incr) { return data_ += incr; } /// @todo Overflow?

private:
//  CountT  data_ = 0;
    struct less : std::binary_function<Node*, Node*, bool> {
        bool operator()(const Node* x, const Node* y) const { return x->iaddr < y->iaddr; }
    };
    std::set<Node*, Node::less>  subs;
};

class Tree : public Node {
public:
    void insert(Node* ip);
};
*/
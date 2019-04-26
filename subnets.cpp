//
// Created by Vladimir Ivanov on 2019-04-23.
//

#include "subnets.h"
#include "trie.h"

#include <set>

#include "trace.h"


//static
//std::ostream& operator<<(std::ostream& os, const Node& node) {
//    return os << *node.ip;
//}

static std::ostream& outline(std::ostream& os, const Node& node, int level = 0) {
    std::string prefix(level, '\t');
    return os << prefix << node << std::endl;
}

static
std::ostream& print_hierarchy(std::ostream& os, const Node& node, int level = 0)
{
    outline(os, node, level);
    for (auto const& x : node.subs)
        print_hierarchy(os, *x, level+1);
    return os;
}


std::ostream& operator<<(std::ostream& os, const Tree& tree) {
    return print_hierarchy(os, tree);
}

IP4Address::IP4Address(const std::string& ip)
{
    in_addr_t   a;
    auto rc = inet_pton(AF_INET, ip.c_str(), &a);
    if (!rc)
        throw std::invalid_argument("Can't convert to in_addr: " + ip.substr(0, 20));
    iaddr = ntohl(a);
}


IP4Network::IP4Network(const std::string& ip, int b)
        : IP4Address(ip)
        , bits((uint8_t)b)
{
    int shift = 32 - bits;
    if (shift != 0) {
        uint32_t mask = ~0u >> shift << shift;
        if (iaddr & ~mask) {
            log_warn << "Invalid IP: host bits are set " << *this;
        //    throw std::invalid_argument("Invalid IP: host bits are set");
            iaddr &= mask;
        //  addr = htonl(iaddr);
        }
    }
//    cout << rc << endl;
}

bool IP4Network::supernetOf(const IP4Address& sub) const
{
    // widest mask is numerically less than the narrow one
    // @todo: use intrinsic for bit ops
    auto b = 32 - bits;
    return (sub.iaddr >> b << b) == iaddr;
}

bool IP4Network::supernetOf(const IP4Network& sub) const
{
    // widest mask is numerically less than the narrow one
    // @todo: use intrinsic for bit ops
    auto b = 32 - bits;
    return (bits <= sub.bits
            && (sub.iaddr >> b << b) == iaddr);
}

bool Node::less::operator()(const Node* x, const Node* y) const {
//    log_trace << *x << ((x->ip->iaddr < y->ip->iaddr) ? " < " : " > ") << *y;
    return x->iaddr < y->iaddr
        || (x->iaddr == y->iaddr && x->bits < y->bits);
}


// tree
void Tree::insert(Node* node)
{
    // Root owns all:
    // When building, the root is just a list of top-level nets
    // When accumelating, root.add means "unknown"

    Node* current = this;
    auto bound = current->subs.end();

    while (!current->subs.empty()) {
        // for current level of nesting
        bound = current->subs.lower_bound(node);
        if (bound == current->subs.begin()) {
            // not found in children, insert into the current level
            break;
        } else if (bound != current->subs.end() && (*bound)->iaddr == node->iaddr) {
            // exact match or subnet?
            if ((*bound)->bits == node->bits) {
                // Check for exact match when building. Ignore (warn) if owner is the same, otherwise raise error
                // Recurse deeper when accumulating. Do nothing here, it's implemented below
                log_error << "IP4Network duplicate: " << **bound;
                return; // skip
            } else if ((*bound)->bits < node->bits) {
                assert((*bound)->supernetOf(*node));
                current = *bound;
                bound = current->subs.end(); // iterator shall be inside of current set. Bad style...
                continue;
            } else {
                assert(node->supernetOf(*(*bound)));
                break;
            }
        } else {
            // one step back over sorted siblings
            assert(bound != current->subs.begin());
            auto prev = std::prev(bound); // bound; --prev; //
            if ((*prev)->supernetOf(*node)) {
                // continue recursion
                current = *prev;
                bound = current->subs.end(); // iterator shall be inside of current set. Bad style...
                continue;
            } else {
                break;
            }
        }
    }

    auto it = bound;
    for ( ; it != current->subs.end() && node->supernetOf(*(*it)); ++it) {
        // move subs from the current to the new one
        node->subs.insert(*it);
    }
    current->subs.erase(bound, it);
    current->subs.insert(node); // use bound as a hint
}


Node* Tree::lookup(const IP4Address& ip)
{
    // Root owns all:
    // When building, the root is just a list of top-level nets
    // When accumelating, root.add means "unknown"

//    Node* node = static_cast<Node*>(const_cast<IP4Address*>(&ip));
    Node n(ip.iaddr);
    Node* node = &n;

    Node* current = this;
    auto bound = current->subs.end();

    while (!current->subs.empty()) {
        // for current level of nesting
        bound = current->subs.lower_bound(node);
        if (bound == current->subs.begin()) {
            // not found in children, insert into the current level
            break;
        } else if (bound != current->subs.end() && (*bound)->iaddr == node->iaddr) {
            // exact match or subnet?
            if ((*bound)->bits == node->bits) {
                // Check for exact match when building. Ignore (warn) if owner is the same, otherwise raise error
                // Recurse deeper when accumulating. Do nothing here, it's implemented below
                return *bound; // skip
            } else if ((*bound)->bits < node->bits) {
                assert((*bound)->supernetOf(*node));
                current = *bound;
                continue;
            } else {
                assert(node->supernetOf(*(*bound)));
                break;
            }
        } else {
            // one step back over sorted siblings
            assert(bound != current->subs.begin());
            auto prev = std::prev(bound); // bound; --prev; //
            if ((*prev)->supernetOf(*node)) {
                // continue recursion
                current = *prev;
                continue;
            } else {
                break;
            }
        }
    }

    return current;
}

/*
void outline(RNode* p, int level)
{
    if (!p)
        return;

    std::cout << std::string(level, '\t') << *p << std::endl;
    outline(p->left, level+1);
    outline(p->right, level+1);
}

void walk(RNode* p, int level)
{
    if (!p)
        return;

    if (p->bits == p->plen) {
        std::cout << std::string(level, '\t') << *(IP4Network*)p << std::endl;
        ++level;
    }
    if (p->left && p->right && p->right->iaddr < p->left->iaddr) {
        walk(p->right, level);
        walk(p->left, level);
    } else {
        walk(p->left, level);
        walk(p->right, level);
    }
}
*/

#ifdef UT_CATCH
#define CATCH_CONFIG_MAIN
#include "catch.hpp"


TEST_CASE( "leftmostbit", "[leftmostbit]")
{
    REQUIRE( leftmostbit(0b1) == 31 ) ;
    REQUIRE( leftmostbit(0b00000000000000000000000000000100) == 29 ) ;
    REQUIRE( leftmostbit(0b10000000010000000000000000000100) == 0 ) ;
}

TEST_CASE( "IP4Network.supernetOf", "[IP4Network]")
{
    IP4Network ip("192.168.0.0", 24);
    REQUIRE( ip.supernetOf( IP4Network("192.168.0.64", 28) )  );
    REQUIRE( ip.supernetOf( IP4Network("10.168.1.16", 8) ) == false );
    REQUIRE( ip.supernetOf( IP4Network("192.168.0.0", 32) ));
    REQUIRE( ip.supernetOf( IP4Network("192.168.0.255", 32) ));
    REQUIRE( ip.supernetOf( IP4Network("192.168.1.16", 31) ) == false );
    IP4Network ip2("67.5.239.0", 24);
    REQUIRE( ip2.supernetOf( IP4Network("67.5.239.16", 28) ) );
}
#endif // UT_CATCH
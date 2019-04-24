//
// Created by Vladimir Ivanov on 2019-04-23.
//

#include "subnets.h"

#include <set>

#include "trace.h"

static
std::ostream& operator<<(std::ostream& os, const Node& node) {
    return os << *node.ip;
}

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


IP4Network::IP4Network(const std::string& ip, int b)
        : bits((uint8_t)b) {
//    unsigned char* a = (unsigned char*)&addr.s_addr;
    auto rc = inet_pton(AF_INET, ip.c_str(), &addr);
    if (!rc)
        throw std::invalid_argument("Can't convert to in_addr: " + ip.substr(0, 20));
    iaddr = ntohl(addr);

    int shift = 32 - bits;
    if (shift != 0) {
        uint32_t mask = ~0u >> shift << shift;
        if (iaddr & ~mask) {
            log_warn << "Invalid IP: host bits are set " << *this;
        //    throw std::invalid_argument("Invalid IP: host bits are set");
            iaddr &= mask;
            addr = htonl(iaddr);
        }
    }
//    cout << rc << endl;
}

bool IP4Network::supernetOf(const IP4Network& sub) const
{
    // widest mask is numerically less than the narrow one
    // @todo: use intrinsic for bit ops
    auto b = 32 - bits;
    return (bits < sub.bits
            && (sub.iaddr >> b << b) == iaddr);
}

bool Node::less::operator()(const Node* x, const Node* y) const {
//    log_trace << *x << ((x->ip->iaddr < y->ip->iaddr) ? " < " : " > ") << *y;
    return x->ip->iaddr < y->ip->iaddr; }

#undef DEF_LOG_LEVEL
#define DEF_LOG_LEVEL (LOG_LEVEL::error)

// tree
void Tree::insert(Node* node)
{
    // Root owns all:
    // When building, the root is just a list of top-level nets
    // When accumelating, root.add means "unknown"

    Node* current = this;
    auto bound = current->subs.end();
TraceX(*node);

int level = 0;
    while (!current->subs.empty()) {
TraceX(level);
++level;
TraceX(1);
TraceX(*current);
TraceX(current->subs.size());
        // for current level of nesting
        bound = current->subs.lower_bound(node);
        if (bound == current->subs.begin()) {
            TraceX(2);
            // not found in children, insert into the current level
            break;
        } else if (bound != current->subs.end() && (*bound)->ip->iaddr == node->ip->iaddr) {
            TraceX(3);
            // exact match or subnet?
            if ((*bound)->ip->bits == node->ip->bits) {
                // Check for exact match when building. Ignore (warn) if owner is the same, otherwise raise error
                // Recurse deeper when accumulating. Do nothing here, it's implemented below
                log_error << "IP4Network duplicate: " << **bound;
                return; // skip
            } else if ((*bound)->ip->bits < node->ip->bits) {
                assert((*bound)->ip->supernetOf(*node->ip));
                current = *bound;
                continue;
            } else {
                assert(node->ip->supernetOf(*(*bound)->ip));
                break;
            }
        } else {
            TraceX(5);
            // one step back over sorted siblings
            assert(bound != current->subs.begin());
            auto prev = std::prev(bound); // bound; --prev; //
            if ((*prev)->ip->supernetOf(*node->ip)) {
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
    for ( ; it != current->subs.end() && node->ip->supernetOf(*(*it)->ip); ++it) {
        // move subs from the current to the new one
        node->subs.insert(*it);
    }
    current->subs.erase(bound, it);
    current->subs.insert(node); // use bound as a hint
    log_trace << "Tree:\n" << (*this);
}


/*
// tree
void Tree::add(IP4Network* node, uint64_t incr)
{
    // Root owns all:
    // When building, the root is just a list of top-level nets
    // When accumelating, root.add means "unknown"

    Node* current = this;
    auto bound = current->subs.end();
    TraceX(*node);
//log_trace << "Tree:\n" << (*this);
//int k = 0; for (auto const& x : current->subs) {
//    Node::less less;
//    log_trace << "current->subs["<<k<<"] = " << *x << (less(x,node) ? " less" : " more");
//}

    int level = 0;
    while (!current->subs.empty()) {
        TraceX(level);
        ++level;
        TraceX(1);
        TraceX(*current);
        TraceX(current->subs.size());
        // for current level of nesting
        Node::less comparator;
        bound = std::lower_bound(current->subs.begin(), current->subs.end(), node, comparator);
        if (bound == current->subs.begin()) {
            TraceX(2);
            // not found in children, insert into the current level
            break;
        } else if (bound != current->subs.end() && (*bound)->ip->iaddr == node->ip->iaddr) {
            TraceX(3);
            // exact match or subnet?
            if ((*bound)->ip->bits == node->ip->bits) {
                // Check for exact match when building. Ignore (warn) if owner is the same, otherwise raise error
                // Recurse deeper when accumulating. Do nothing here, it's implemented below
                log_error << "IP4Network duplicate: " << **bound;
                return; // skip
            } else if ((*bound)->ip->bits < node->ip->bits) {
                assert((*bound)->ip->supernetOf(*node->ip));
                current = *bound;
                continue;
            } else {
                assert(node->ip->supernetOf(*(*bound)->ip));
                break;
            }
        } else {
            TraceX(5);
            // one step back over sorted siblings
            assert(bound != current->subs.begin());
            auto prev = std::prev(bound); // bound; --prev; //
            if ((*prev)->ip->supernetOf(*node->ip)) {
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
    for ( ; it != current->subs.end() && node->ip->supernetOf(*(*it)->ip); ++it) {
        // move subs from the current to the new one
        node->subs.insert(*it);
    }
    current->subs.erase(bound, it);
    current->subs.insert(node); // use bound as a hint
    log_trace << "Tree:\n" << (*this);
}
*/

#ifdef UT_CATCH
#include "catch.hpp"

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
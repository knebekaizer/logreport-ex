//
// Created by Vladimir Ivanov on 2019-04-23.
//

#include "subnets.h"

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

#undef DEF_LOG_LEVEL
#define DEF_LOG_LEVEL (LOG_LEVEL::debug)

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
            TraceX(2);
            // not found in children, insert into the current level
            break;
        } else if (bound != current->subs.end() && (*bound)->iaddr == node->iaddr) {
            TraceX(3);
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
            TraceX(5);
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
        } else if (bound != current->subs.end() && (*bound)->iaddr == node->iaddr) {
            TraceX(3);
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
            TraceX(5);
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

struct IP4N {
    uint32_t a;
    uint8_t bits;
};

class RNode : public IP4N
{
public:
    uint32_t mask;
    RNode* left;
    RNode* right;
    uint8_t plen;
};


class Radix
{
    RNode* lookup(const IP4Network& ip) const;
    void insert(const IP4Network& ip);
};

RNode*
Radix::lookup(const IP4Network& ip) const
{
    RNode* c = root;
    while (c->zero) { // has child?
        if (c->zero->supernetOf(ip)) {
            c = c->zero;
        } else if (c->one->supernetOf(ip)) {
            c = c->one;
        } else {
            break;
        }
    }
}

uint32_t mask(uint8_t bits)
{
    return ~0u << (32 - bits);
}


void
Radix::insert(const IP4N& i)
{
    RNode* p = lookup(ip); // parent

    // ip.bits > c.bits
    // if no children then add ip as  child
    // if one child has common prefix then split it
    // otherwise add ip as a child

    int bits; // length of network mask = number of network bits
    int plen; // length of path to the current node (ibn bits); plen <= nbits

    if (p->left) {
        auto c = p->left;
        auto x = (c.a ^ i.a) & ~mask(p->plen); // clear first p->plen bits
        if (x) { // C and I have common prefix; split them
            if (i.bits < c.plen && (x & mask(i.bits) == 0)) {
                // insert i as super of c
            } else if (c.plen < i.bits && (x & mask(c.plen) == 0)) {
                // c is super of i; recurse further
            }
        }
    }

/*
        if p->left and i have common prefix
            split left:
                c = left;
                x = c ^ i
                for (n = p.plen+1; n<i.nbits && n < c->plen) {}
                    if bit(x, n)  break // n-th bit differ
                }
                // now n points next to common bits
                if n == c->plen
                    then recurse further
                if n == i->nbits // i is super of c
                    then make i a new child of P and move c to child of i
                else if n < i->nbits
                    then split:
                        create nc as new_child(i, n)
                        move c to child of nc
                        set i as second child of nc

        else if p->right {
            assert p->right and i have common prefix;
            split right;
        } else {
            p->right = i;
        }


    } else {
        p->left = i;
    }
*/
    auto pm = p->mask;
    // len is length of parent mask

    int next = len + 1;

    auto i = ip.iaddr;

    diff = (i ^ other) & ~pm;
    while (bitset(diff, len)) {
        ++len;
    }
    uint32_t tail = i & ~pm;

}


#ifdef UT_CATCH
#define CATCH_CONFIG_MAIN
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
/// @file

#include "subnets.h"
#include "trie.h"

#include <set>

#include "trace.h"

using std::string;


void IP::parse(const string& s)
{
	in_addr_t   a;
	auto rc = inet_pton(AF_INET, s.c_str(), &a);
	if (!rc) {
		throw std::invalid_argument("Can't convert to in_addr: " + s.substr(0, 20));
	}
	addr = ntohl(a);

	if (bits < addr_size) {
		uint32_t mask = ~0U >> bits;
		if (addr & mask) {
			log_warn << "Invalid IP: host bit set: [" << *this <<"] Host bits are ignored";
			//    throw std::invalid_argument("Invalid IP: host bits are set");
			addr &= ~mask;
		}
	}
}

IP::IP(string s)
{
	auto pos = s.find('/');
	if (pos != string::npos) {
		const char* p = s.c_str();
		char* e = 0;
		bits = strtol(p + pos + 1, &e, 10); /// @todo check format: any extra symbols
		if (bits == 0 || bits > addr_size || e != p + s.size()) {
			throw std::invalid_argument("Invalid network mask: " + s);
		}
		s.erase(pos);
	}
	// 	bits defaulted to addr_size in class declaration

	parse(s);
}

IP::IP(string s, int b)
	: bits((MaskT)b)
{
	parse(s);
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

IP4Address::IP4Address(const std::string& ip)
{
    in_addr_t   a;
    auto rc = inet_pton(AF_INET, ip.c_str(), &a);
    if (!rc)
        throw std::invalid_argument("Can't convert to in_addr: " + ip.substr(0, 20));
    addr = ntohl(a);
}


IP4Network::IP4Network(const std::string& ip, int b)
        : IP4Address(ip)
        , bits((uint8_t)b)
{
    int shift = 32 - bits;
    if (shift != 0) {
        uint32_t mask = ~0u >> shift << shift;
        if (addr & ~mask) {
            log_warn << "Invalid IP: host bit set: [" << *this <<"] Host bits are ignored";
        //    throw std::invalid_argument("Invalid IP: host bits are set");
            addr &= mask;
        }
    }
}

bool IP4Network::supernetOf(const IP4Address& sub) const
{
    // widest mask is numerically less than the narrow one
    // @todo: use intrinsic for bit ops
    auto b = 32 - bits;
    return (sub.addr >> b << b) == addr;
}

bool IP4Network::supernetOf(const IP4Network& sub) const
{
    // widest mask is numerically less than the narrow one
    // @todo: use intrinsic for bit ops
    auto b = 32 - bits;
    return (bits <= sub.bits
            && (sub.addr >> b << b) == addr);
}

bool Node::less::operator()(const Node* x, const Node* y) const {
//    log_trace << *x << ((x->ip->addr < y->ip->addr) ? " < " : " > ") << *y;
    return x->addr < y->addr
        || (x->addr == y->addr && x->bits < y->bits);
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
        } else if (bound != current->subs.end() && (*bound)->addr == node->addr) {
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
    Node n(ip.addr);
    Node* node = &n;

    Node* current = this;
    auto bound = current->subs.end();

    while (!current->subs.empty()) {
        // for current level of nesting
        bound = current->subs.lower_bound(node);
        if (bound == current->subs.begin()) {
            // not found in children, insert into the current level
            break;
        } else if (bound != current->subs.end() && (*bound)->addr == node->addr) {
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


#ifdef UT_CATCH
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cstdlib>

TEST_CASE( "IP.parse", "[IP]")
{
	for (int k=0; k<16; ++k) {
		int bits = (1u + (uint8_t) rand()) % 32;
		uint32_t a = (uint32_t) (rand() & (~0u << (32 - bits)));
		char buf[20];
		snprintf(buf, sizeof(buf), "%d.%d.%d.%d/%d", (a >> 24) & 0xff, (a >> 16) & 0xff, (a >> 8) & 0xff, a & 0xff,
		         bits);
		IP ip(buf);
		REQUIRE(ip.addr == a);
		REQUIRE(ip.bits == bits);
	}

	int err_count = 0;
	std::string bad[] = {
			"10.168.1.16/255",
			"10.168.1.16/0",
			"10.168.1.16/1O", // letter "O" instead of digit 0
			"512.168.1.16/8"
	};
	for (auto& s : bad) {
		try {
			IP ip(s);
		}
		catch (std::exception &e) {
			log_info << "Exception test: " << e.what();
			++err_count;
		}
	}
	REQUIRE(err_count == sizeof(bad) / sizeof(*bad));
};

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
    REQUIRE( ip.supernetOf( IP4Network("10.168.1.16", 8) ) == false );  // this shall produce WARN log
    REQUIRE( ip.supernetOf( IP4Network("192.168.0.0", 32) ));
    REQUIRE( ip.supernetOf( IP4Network("192.168.0.255", 32) ));
    REQUIRE( ip.supernetOf( IP4Network("192.168.1.16", 31) ) == false );
    IP4Network ip2("67.5.239.0", 24);
    REQUIRE( ip2.supernetOf( IP4Network("67.5.239.16", 28) ) );
}
#endif // UT_CATCH
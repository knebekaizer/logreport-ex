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
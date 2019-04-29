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
	addr_ = ntohl(a);

	if (bits < addr_size) {
		uint32_t mask = ~0U >> bits;
		if (addr_ & mask) {
			log_warn << "Invalid IP: host bit set: [" << *this <<"] Host bits are ignored";
			//    throw std::invalid_argument("Invalid IP: host bits are set");
			addr_ &= ~mask;
		}
	}
}

IP::IP(string s)
{
	auto pos = s.find('/');
	if (pos != string::npos) {
		const char* p = s.c_str();
		char* e = 0;
		bits = strtol(p + pos + 1, &e, 10);
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

IPv6::IPv6(string s)
{
	auto pos = s.find('/');
	if (pos != string::npos) {
		const char* p = s.c_str();
		char* e = 0;
		bits = strtol(p + pos + 1, &e, 10);
		if (bits == 0 || bits > addr_size || e != p + s.size()) {
			throw std::invalid_argument("Invalid network mask: " + s);
		}
		s.erase(pos);
	}
	// 	bits defaulted to addr_size in class declaration

	parse(s);
}

void IPv6::parse(const string& s)
{
//	assert(sizeof(addr_) == addr_size / 8;
	uint8_t* a = (uint8_t*)&addr_;
	auto rc = inet_pton(AF_INET6, s.c_str(), a);
	if (!rc) {
		throw std::invalid_argument("Can't convert to in_addr: " + s.substr(0, 20));
	}

	if (bits < addr_size) {
		auto i = bits >> 3;
		int err = 0;
		if (bits & 7) {
			uint8_t mask = 0xff >> (bits & 7);
			if (a[i] & mask) {
				//    throw std::invalid_argument("Invalid IP: host bits are set");
				a[i] &= ~mask;
				++err;
			}
		}
		for (i += 1; i < addr_size>>3; ++i) {
			if (a[i]) {
				a[i] = 0;
				++err;
			}
		}
		if (err) {
			log_warn << "Invalid IP: host bit set: [" << *this << "] Host bits are ignored ";
		}
	}

//	addr_.hi = ntohll(addr_.hi);
//	addr_.lo = ntohll(addr_.lo);
}


inline
std::ostream& operator<<(std::ostream& os, const IPv6& ip)
{
	char buf[40];
	auto a = ip.addr();
	if (inet_ntop(AF_INET6, &a, buf, sizeof(buf))) {
		os << buf;
	} else {
		os << "Address conversion failure";
	}
	if (ip.size() < IPv6::addr_size)
		os << "/" << (int)ip.size();
	return os;
}


inline
std::ostream& operator<<(std::ostream& os, const IP& ip)
{
	auto a = ip.addr();
	os << (a>>24) << '.' << ((a>>16) & 0xff) << '.' << ((a>>8) & 0xff) << '.' << (a & 0xff) << '/' << (int)ip.size();
	return os;
}


#ifdef UT_CATCH
#include "catch.hpp"

#include <cstdlib>
#include <string.h>

TEST_CASE( "IP.parse", "[IP]")
{
	for (int k=0; k<16; ++k) {
		int bits = (1u + (uint8_t) rand()) % 32;
		uint32_t a = (uint32_t) (rand() & (~0u << (32 - bits)));
		char buf[20];
		snprintf(buf, sizeof(buf), "%d.%d.%d.%d/%d", (a >> 24) & 0xff, (a >> 16) & 0xff, (a >> 8) & 0xff, a & 0xff,
		         bits);
		IP ip(buf);
		REQUIRE(ip.addr() == a);
		REQUIRE(ip.size() == bits);
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


TEST_CASE( "IPv6.parse", "[IPv6]")
{
	uint8_t a[16] = {
		0x2b, 0xab, 0xbe, 0x0d, 0x8b, 0x6d, 0xb4, 0xa2, 0xbe, 0xf7, 0xb9, 0x7c, 0xf7, 0x97, 0x5f, 0xc0
	};
	REQUIRE(memcmp(&IPv6("2bab:be0d:8b6d:b4a2:bef7:b97c:f797:5fc0/122").addr()[0], a, 16) == 0);

//	load> IPv6(netw) = 2bab:be0d:8b6d:b4a2:bef7:b97c:f797:5fc0/122
//load> IPv6(netw) = 2776:f1ab:eff9:8000::/56
//load> IPv6(netw) = 301a:48d3:fc05:32de:8972:8c74:2dd5:6800/117
//load> IPv6(netw) = 2bab:be0d:8b6d:b4a2:bef7:b97c:f797:5fc4/127
//load> IPv6(netw) = 3ff0:ab45:9000::/37
//load> IPv6(netw) = 3672:ab4a:8172:3800::/57

}


TEST_CASE( "leftmostbit", "[leftmostbit]")
{
    REQUIRE( leftmostbit(0b1) == 31 ) ;
    REQUIRE( leftmostbit(0b00000000000000000000000000000100) == 29 ) ;
    REQUIRE( leftmostbit(0b10000000010000000000000000000100) == 0 ) ;
}

/*
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
*/
#endif // UT_CATCH
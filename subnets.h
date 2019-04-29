/// @file

#ifndef IPLOG_TEST_SUBNETS_H
#define IPLOG_TEST_SUBNETS_H

#include <cstdint>
#include <set>

#include <iostream>
#include <string>
#include <array>

#include <arpa/inet.h>

#include "trace.h"


class IP {
public:
	IP() = default;
	explicit IP(std::string s);  // s by value
	explicit IP(std::string s, int bits);

	using AddrT = uint32_t;
	using MaskT = uint8_t;
	static constexpr auto addr_size = 32;
	AddrT addr() const { return addr_; }
	MaskT size() const { return bits; }

	bool operator==(const IP& other) const {
		return addr_ == other.addr_ && bits == other.bits;
	}
private:
	void parse(const std::string& s);

	AddrT addr_ = 0;
	MaskT bits = addr_size;  ///< network mask length
};


class IPv6 {
public:
	IPv6() = default;
	explicit IPv6(std::string s);  // s by value
/*
	struct AddrT {
		uint64_t hi, lo;
		bool operator==(const AddrT& other) const {
			return hi == other.hi && lo == other.lo;
		}
	};
*/
	using AddrT = std::array<uint8_t, 16>;
	using MaskT = uint8_t;
	static constexpr auto addr_size = sizeof(AddrT) * 8;
	static constexpr auto chunk_size = 8;
	const AddrT& addr() const { return addr_; }
	MaskT size() const { return bits; }

	bool operator==(const IPv6& other) const {
		return addr_ == other.addr_ && bits == other.bits;
	}
private:
	void parse(const std::string& s);

	AddrT addr_ = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	MaskT bits = addr_size;  ///< network mask length
};

std::ostream& operator<<(std::ostream& os, const IP& ip);
std::ostream& operator<<(std::ostream& os, const IPv6& ip);




/// @return 0 or 1 only (nefer 0b100).
inline unsigned int bit(IP::AddrT x, unsigned int bit)
{
	assert(bit <= IP::addr_size - 1);
	return 1 & (x >> (IP::addr_size - 1 - bit));
}

/// @return 0 or 1 only (nefer 0b100).
inline unsigned int bit(IPv6::AddrT a, unsigned int bit)
{
	assert(bit <= IPv6::addr_size - 1);
	return 1 & (a[bit >> 3] >> (7 - (bit & 7)));
}

#endif //IPLOG_TEST_SUBNETS_H

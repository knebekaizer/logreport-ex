//
// Created by Володя on 2019-04-29.
//

#ifndef IPLOG_TEST_IPSUMMARY_H
#define IPLOG_TEST_IPSUMMARY_H

#include "trace.h"
#include "err_policy.h"
#include "trie.h"
#include "subnets.h"

#include <iostream>
#include <fstream>
#include <string>
#include <memory> // make_unique
#include <vector>
#include <set>

#include <boost/multiprecision/cpp_int.hpp>
namespace mp = boost::multiprecision;

class Payload {
public:
	using CountT = mp::uint128_t;
	void incr(uint64_t x) { data_ += x; }
	CountT data() const { return data_; }
//	operator const CountT() const { return data(); }
private:
	CountT data_ = 0;
};

struct RegElem {
	RegElem(const std::string& name)
			: id(name)
			  , data(std::make_unique<Payload>())
	{}

	struct less : std::binary_function<RegElem, RegElem, bool> {
		bool operator()(const RegElem& x, const RegElem& y) const { return x.id < y.id; }
	};

	std::string id;
	std::unique_ptr<Payload> data;
};

class IpSummary
{
public:
	IpSummary(int argc, const char * argv[]) { argsParser(argc, argv); }
	int initData();
	int processLog();
	int printReport();

	int run();

private:
	int argsParser(int argc, const char * argv[]);
	std::vector<std::string> args;

	int load(std::istream& is);
	int processData(std::istream& is);
	int report(std::ostream& os) const;

	std::set<RegElem, RegElem::less>  registry;
	Payload unknown;

#ifdef UT_CATCH
public:
#endif
	trie::Radix<IP> trie;
	trie::Radix<IPv6> trie_v6;

#ifdef IPLOG_SELFTEST
	mutable struct {
		void accum1(Payload::CountT x) { sum1 += x; }
		void accum2(Payload::CountT x) { sum2 += x; }
		void validate() {
			require(sum1 == sum2) << " with sum1 = " << sum1 << "; sum2 = " << sum2;
			log_info << "[SelfTest]  Total sum validation passed";
		}
	private:
		Payload::CountT sum1 = 0;
		Payload::CountT sum2 = 0;
	} sumCheck;
	void sumCheck_accum1(Payload::CountT n) const { sumCheck.accum1(n); }
	void sumCheck_accum2(Payload::CountT n) const { sumCheck.accum2(n); }
	void sumCheck_validate() const { sumCheck.validate(); }
#else
	#define sumCheck_accum1(n)
#define sumCheck_accum2(n)
#define sumCheck_validate()
#endif

};

#endif //IPLOG_TEST_IPSUMMARY_H

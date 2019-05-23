//
// Created by Володя on 2019-04-29.
//

#include "IpSummary.h"

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
#include <chrono>


using namespace std;

ostream& operator<<(ostream& os, const RegElem& x) {
	return os << x.id << " " << x.data->data();
}


template<class T>
ostream& operator<<(ostream& os, const vector<T>& v) {
	for (const auto& x : v) {
		os << x << endl;
	}
	return os;
}


template<class T>
ostream& operator<<(ostream& os, const set<T>& v) {
	for (const auto& x : v) {
		os << x << endl;
	}
	return os;
}


using namespace std::chrono;
class TimeHelper {
public:
	using time_t = time_point<high_resolution_clock>;
	TimeHelper() : start(high_resolution_clock::now()) {}
	auto operator()() const { return high_resolution_clock::now() - start; }
	auto mksec() const { return duration_cast<microseconds>((*this)()).count(); }
private:
	time_t start;
};


int IpSummary::load(istream& is)
{
	string id; // customer id
	string netw; // text repr of the network
	trie.root.data = &unknown;
	trie_v6.root.data = &unknown;

	// @todo use getline to validate input format
	while (is >> netw >> id) {
		try {
			if (netw == "ERROR:") {
				// special case: handle a sed filter error which is sent from subshell
				throw std::invalid_argument("");
			}
			auto elem = registry.emplace(id).first;
			if (netw.find(':') != string::npos) {
				// ipv6
				auto node __attribute__((unused))= trie_v6.insert(IPv6(netw), elem->data.get());
				assert(node->ip == IPv6(netw));
			} else {
				auto node __attribute__((unused))= trie.insert(IP(netw), elem->data.get());
				assert(node->ip == IP(netw));
			}
		}
		catch (std::exception& e) {
			log_error << "Error on loading customers data: Bad format at line " << registry.size() + 1 << " " << e.what();
			is.setstate(std::ios_base::badbit);
			//    break;
		}
	}

//	log_trace << registry;

	log_trace << "Done " << registry.size() << " lines";
	// check eof and failbit. Stop ib case of failbit but not eof, as it means the stream is heavily misformatted
	if (is.bad()) return 1;

	//    log_trace << "Tree:\n" << tree;

#ifdef IPLOG_SELFTEST
	trie.selfTest_run();
#endif

//    outline(&trie.root);
//    log_trace << trie;
//	log_trace << trie_v6;
	return 0;
}


int IpSummary::processData(istream& is)
{
	string ip;
	uint64_t bytes;
	uint64_t line = 1;
	while (is >> ip >> bytes) {
;		if (ip.find(':') != string::npos) {
			auto p = trie_v6.lookup(IPv6(ip));
			require(p->data) << " with *p = " << *p;
			p->data->incr(bytes);
		} else {
			auto p = trie.lookup(IP(ip));
			require(p->data) << " with *p = " << *p;
			p->data->incr(bytes);
		}

		sumCheck_accum1(bytes);
		++line;
	}
	if (!is.eof()) {
		log_error << "Error on loading ip log: Bad format at line " << line;
		throw std::runtime_error("Bad format");
		return 1;
	}
	return 0;
}

int IpSummary::report(ostream& os) const
{
	string last;
	for (auto& x : registry) {
		if (x.data->data()) {
			os << x << std::endl;
			sumCheck_accum2(x.data->data());
		}
	}

	if (unknown.data()) {
		os << "UNKNOWN" << " " << unknown.data() << endl;
		sumCheck_accum2(unknown.data());
	}
	sumCheck_validate();

	return os ? 0 : 1;
}


int IpSummary::initData(const string file)
{
	TimeHelper time;
	int err = 0;
	if (file == "-") {
		err = load(std::cin);
	} else {
		ifstream is(file);
		if (!is) {
			log_error << "Open error: " << file;
			return -1;
			// throw std::runtime_error("Open error: " + file);
		}
		err = load(is);
	}
	log_info << "Time elapsed: " << time.mksec() << " mksec";

	return err;
};

int IpSummary::processLog(const string file)
{
	TimeHelper time;
	int err = 0;
	if (file == "-") {
		err = processData(std::cin);
	} else {
		ifstream is(file);
		if (!is) {
			throw std::runtime_error("Open error: " + file);
		}
		err = processData(is);
	}
	log_info << "Time elapsed: " << time.mksec() << " mksec";

	return err;
};

int IpSummary::printReport(const string file)
{
	TimeHelper time;
	int err = 0;
	if (file == "-") {
		err = report(std::cout);
	} else {
		ofstream os(file);
		if (!os) {
			throw std::runtime_error("Open error: " + file);
		}
		err = report(os);
	}
	log_info << "Time elapsed: " << time.mksec() << " mksec";
	return err;
};

int IpSummary::run(std::vector<std::string> args)
{
	try {
		log_info << "arguments:\n" << args;
		int err =  initData(args.at(0))
		           || processLog(args.at(1))
		           || printReport(args.at(2));
		return err;
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	catch (...) {
		log_fatal << "Unknown exception";
	}
	return -1;
}


#ifdef UT_CATCH
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE( "Trie.lookup", "[Trie]")
{
    IpSummary ipr;
    int rc = ipr.initData("test/data/c5.txt");
    if (rc) {
        log_warn << "Initialization error, test is skipped ";
	    return;
	}

	vector<pair<IP,IP>> data = {
			{IP("239.254.94.1"), IP("239.254.94.0/23")},
			{IP("239.254.1.2"),  IP("239.254.0.0/15")},
			{IP("239.248.0.2"),  IP("239.248.0.0/13")},
			{IP("239.255.235.108"), IP("239.254.0.0", 15)}
	};

	for (const auto& x : data) {
		bool cond = ipr.trie.lookup(x.first)->ip == x.second;
		if (!cond) {
			log_error << "Test failed for " << x.first << " in " << x.second;
		}
		REQUIRE(cond);
	}

}

#endif // UT_CATCH

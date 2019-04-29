#include <iostream>
#include <fstream>
#include <string>
#include <memory> // make_unique

#include <arpa/inet.h>

#include "trace.h"
#include "trie.h"
#include "subnets.h"


#include <vector>
#include <set>

using namespace std;

class Payload {
public:
	using CountT = uint64_t;
	void incr(uint64_t x) { data_ += x; }
	CountT data() const { return data_; }
//	operator const CountT() const { return data(); }
private:
	uint64_t data_ = 0;
};

struct RegElem {
    RegElem(const string& name)
        : id(name)
        , data(std::make_unique<Payload>())
        {}

	struct less : std::binary_function<RegElem, RegElem, bool> {
		bool operator()(const RegElem& x, const RegElem& y) const { return x.id < y.id; }
	};

	string id;
    unique_ptr<Payload> data;
};

ostream& operator<<(ostream& os, const RegElem& x) {
	return os << x.id << "\t" << x.data->data();
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


#include "err_policy.h"

class IPRegistry
{
public:
    int load(istream& is);
    istream& processData(istream& is);
    ostream& report(ostream& os) const;

	std::set<RegElem, RegElem::less>  registry;

	trie::Radix trie;
	trie::Radix trie_v6;

	Payload unknown;

#ifdef IPLOG_SELFTEST
	mutable struct {
		void accum1(uint64_t x) { sum1 += x; }
		void accum2(uint64_t x) { sum2 += x; }
		void validate() {
			require(sum1 == sum2) << " with sum1 = " << sum1 << "; sum2 = " << sum2;
			log_info << "[SelfTest]  Total sum validation passed";
		}
	private:
		uint64_t sum1 = 0;
		uint64_t sum2 = 0;
	} sumCheck;
	void sumCheck_accum1(uint64_t n) const { sumCheck.accum1(n); }
	void sumCheck_accum2(uint64_t n) const { sumCheck.accum2(n); }
	void sumCheck_validate() const { sumCheck.validate(); }
#else
#define sumCheck_accum1(n)
#define sumCheck_accum2(n)
#define sumCheck_validate()
#endif

};


int IPRegistry::load(istream& is)
{
    string id; // customer id
    string netw; // text repr of the network
    trie.root.data = &unknown;

	// @todo use getline to validate input format
    while (is >> id >> netw) {
        try {
	        auto elem = registry.emplace(id).first;
	        if (netw.find(':') != string::npos) {
        		// ipv6
        		TraceX(IPv6(netw));
        	} else {
		        auto node = trie.insert(IP(netw));
		        assert(node->ip == IP(netw));
		        node->data = elem->data.get();
	        }
        }
        catch (std::exception& e) {
            log_error << "Bad format: line " << registry.size() + 1 << " " << e.what();
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

    return 0;
}

//#define Tree
istream& IPRegistry::processData(istream& is)
{
    string ip;
    uint64_t bytes;
    uint64_t line = 1;
    while (is >> ip >> bytes) {

		auto p = trie.lookup(IP(ip));
        require(p->data) << " with *p = " << *p;
        p->data->incr(bytes);
        sumCheck_accum1(bytes);

        ++line;
    }
    if (!is.eof()) {
        log_error << "Bad format: line " << line;
        throw std::runtime_error("Bad format");
    }
    return is;
}

ostream& IPRegistry::report(ostream& os) const
{
    string last;
    for (auto& x : registry) {
	    if (x.data->data()) {
		    os << x << std::endl;
		    sumCheck_accum2(x.data->data());
	    }
    }

    assert(trie.root.data);
    if (trie.root.data->data()) {
	    os << "Unknown" << "\t" << trie.root.data->data() << endl;
	    sumCheck_accum2(trie.root.data->data());
    }
    sumCheck_validate();

    return os;
}


int initData(IPRegistry &ipr, const string& customersFile)
{
    try {
        ifstream is(customersFile);
        if (!is) {
            log_error << "Open error: " << customersFile;
            return -1;
        }

        auto rc = ipr.load(is);
        if (rc)
            return 1;


        ifstream ipdata("test/data/iplog.dat");
        ipr.processData(ipdata);

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    catch (...) {
        log_fatal << "Uncaught exception";
    }
    return -1;
}

class IpSummary {
public:
	IpSummary(int argc, const char * argv[]) { argsParser(argc, argv); }
	void initData(istream& is);
	void processLog(istream& is);
	void printReport(ostream& os);
//private:
	void argsParser(int argc, const char * argv[]);
	vector<std::string> args;
};


void IpSummary::argsParser(int argc, const char * argv[])
{
	if (argc > 1 && argc < 5) {
		int k = 1;
		for ( ; k < argc; ++k) {
			args.emplace_back(argv[k]);
		}
		for ( ; k < 4; ++k) {
			args.emplace_back("-");
		}
	} else {
		cout << "Usage: " << argv[0] << " <input_1> <input_2> <output>" << endl;
		cout <<
		     "Parameters are file names or \"-\" for standart input or output,"
		     "  last two may be omitted.\n"
		     "  input_1: Customers database\n"
		     "  input_2: IP log\n"
		     "  output:  Traffic summary by customer ID\n"
		     << endl;
	}
}

#ifndef UT_CATCH
int main (int argc, const char * argv[])
{
	try {
		IpSummary ips(argc, argv);

		if (ips.args.empty())
			return 1;

		IPRegistry ipr;
		auto err = initData(ipr, ips.args[0]);
		if (err) return err;

		if (ips.args[2] == "-") {
			ipr.report(cout);
		} else {
			ofstream out(ips.args[2]);
			if (!out) {
				throw std::runtime_error("File open error: " + ips.args[2]);
			}
			ipr.report(out);
		}
		return err;
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}

}
#endif // CATCH_CONFIG_MAIN


#ifdef UT_CATCH
#include "catch.hpp"
/*
TEST_CASE( "Trie.lookup", "[Trie]")
{
    IPRegistry ipr;
    int rc = initData(ipr);
    if (rc) {
    	log_error << "Initialization error";
    	exit(1);
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
*/
/*
TEST_CASE( "IP4Address.lookup", "[IP4Address]")
{
    Tree tree;
    init(tree);
    IP4Address ip("239.254.0.0");
    Node* p1 = tree.lookup(ip);
    auto correct = ( (p1->addr == (239<<24) + (254<<16)) && p1->bits == 15);
    REQUIRE( correct );
}
*/

#endif // UT_CATCH
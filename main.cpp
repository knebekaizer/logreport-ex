#include <iostream>
#include <fstream>
#include <string>
#include <memory> // make_unique

#include <arpa/inet.h>

#include "trace.h"
#include "trie.h"
#include "subnets.h"

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
    RegElem(const string& name, const string& addr, int bits)
        : id(name), node(std::make_unique<Node>(addr, bits)) {}

    string id;
    std::unique_ptr<Node> node;

    Payload data;
};

#include <vector>

class IPRegistry : public std::vector<RegElem>
{
public:
    int load(istream& is);
    istream& processData(istream& is);
    ostream& report(ostream& os) const;
    Tree tree;
	trie::Radix trie;


#ifdef IPLOG_SELFTEST
	void selfTest();
#else
#define selfTest()
#endif

};


int IPRegistry::load(istream& is)
{
    string id; // customer id
    string netw; // text repr of the network
    int bits;
    // use getline to validate input format
    while (is >> id >> netw) {
        bits = 32;
        try {
            auto pos = netw.find('/');
            if (pos != string::npos) {
                bits = strtol(netw.c_str() + pos + 1, 0, 10); /// @todo check format: any extra symbols
                if (bits == 0) {
	                throw std::invalid_argument(netw);
                }
                netw.erase(pos);
            }
            push_back({id, netw, bits});
            //  log_trace << lines << ": " << netw << "|" << bits;
        }
        catch (std::exception& e) {
            log_error << "Bad format: line " << size() + 1 << " " << e.what();
            is.setstate(std::ios_base::badbit);
            //    break;
        }

    }
    std::sort(begin(), end(), [](const RegElem& x, const RegElem& y){ return x.id < y.id; } );
    log_trace << "Done " << size() << " lines";
    // check eof and failbit. Stop ib case of failbit but not eof, as it means the stream is heavily misformatted
    if (is.bad()) return 1;

    for (auto& x : *this) {
        tree.insert(x.node.get());
    }
//    log_trace << "Tree:\n" << tree;

    for (auto& x : *this) {
        auto node = trie.insert(IP(*x.node.get()));
        assert(node->ip == IP(*x.node.get()));
        node->data = &x.data;
    }

    selfTest();

//    outline(&trie.root);
//    log_trace << trie;

    return 0;
}

#ifdef IPLOG_SELFTEST
void IPRegistry::selfTest()
{
	for (auto const& x : *this) {
		IP ip(*x.node.get());
	    auto f = trie.lookup(ip);
	    bool cond = (f->addr() == ip.addr && f->size() == ip.size());
	    if (!cond) {
		    log_error << "Failed condition (f->addr() == x.addr && f->size() == x.size()) with "
		                 "f = " << *f << "; x = " << ip;
		//    outline(&root);
	    }

	    assert(f->addr() == ip.addr && f->size() == ip.size());
	}
	log_info << "Trie integrity check passed";
}
#endif

//#define Tree
istream& IPRegistry::processData(istream& is)
{
    string ip;
    uint64_t bytes;
    uint64_t line = 1;
    while (is >> ip >> bytes) {

        Node* n = tree.lookup(ip);
        n->incr(bytes);

        Payload* p = trie.lookup(IP(ip))->data;
        assert(p);
        p->incr(bytes);

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
    uint64_t sum = 0;
    string last;
    for (auto& x : *this) {
        if (last != x.id) {
            if (!last.empty() && sum)
                os << last << "\t" << sum << endl;
            sum = 0;
            last = x.id;
        }
//#ifdef Tree
//        sum += x.node->data_;
//#else
        sum += x.data.data();
//#endif
    }
    if (!last.empty() && sum)
        os << last << "\t" << sum << endl;

    os << "Unknown" << "\t" << tree.data_ << endl;

    return os;
}


int initData(IPRegistry &ipr)
{
    try {
    	string customersFile = "test/data/c5.txt";
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
        log_error << e.what();
    }
    catch (...) {
        log_fatal << "Uncaught exception";
    }
    return -1;
}

class IpSummary {
public:
	void initData(istream& is);
	void processLog(istream& is);
	void printReport(ostream& os);
};

#ifndef UT_CATCH
int main()
{
    IPRegistry ipr;
    auto rc = initData(ipr);

//    ofstream os("trie_report_5.txt");
    ipr.report(cout);

    return rc;
}
#endif // CATCH_CONFIG_MAIN


#ifdef UT_CATCH
#include "catch.hpp"

TEST_CASE( "Trie.lookup", "[Trie]")
{
    IPRegistry ipr;
    int rc = initData(ipr);
    if (rc) {
    	log_error << "Initialization error";
    	exit(1);
    }

	vector<pair<IP,IP>> data = {
			{IP({"239.254.94.1", 32}), IP({"239.254.94.0", 23})},
			{IP({"239.254.1.2", 32}),  IP({"239.254.0.0", 15})},
			{IP({"239.248.0.2", 32}), IP({"239.248.0.0", 13})},
			{IP({"239.255.235.108", 32}), IP({"239.254.0.0", 15})}
	};

	for (const auto& x : data) {
		bool cond = ipr.trie.lookup(x.first)->ip == x.second;
		if (!cond) {
			log_error << "Test failed for " << x.first << " in " << x.second;
		}
		REQUIRE(cond);
	}
}

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
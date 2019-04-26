#include <iostream>
#include <fstream>
#include <string>
#include <memory> // make_unique

#include <arpa/inet.h>

#include "trace.h"
#include "trie.h"
#include "subnets.h"

using namespace std;

struct RegElem {
    RegElem(const string& name, const string& addr, int bits)
        : id(name), node(std::make_unique<Node>(addr, bits)) {}

    string id;
    std::unique_ptr<Node> node;
};

#include <vector>

class IPRegistry : public std::vector<RegElem>
{
public:
    int load(istream& is);
    istream& processData(istream& is);
    ostream& report(ostream& os) const;
    Tree tree;
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
                if (bits == 0)
                    throw std::invalid_argument(netw);
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
    log_info << "Done " << size() << " lines";
    // check eof and failbit. Stop ib case of failbit but not eof, as it means the stream is heavily misformatted
    if (is.bad()) return 1;

    for (auto& x : *this) {
        tree.insert(x.node.get());
    }
    log_trace << "Tree:\n" << tree;

    trie::Radix trie;
    for (auto& x : *this) {
        TraceX(*x.node.get());
        trie.insert(IP(*x.node.get()));
    }
    outline(&trie.root);
    walk(&trie.root);

    return 0;
}

istream& IPRegistry::processData(istream& is)
{
    string ip;
    uint64_t bytes;
    uint64_t line = 1;
    while (is >> ip >> bytes) {
        Node* n = tree.lookup(ip);
        n->incr(bytes);
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
        sum += x.node->data_;
    }
    if (!last.empty() && sum)
        os << last << "\t" << sum << endl;

    os << "Unknown" << "\t" << tree.data_ << endl;

    return os;
}


int init(IPRegistry& ipr)
{
    try {
        ifstream is("test/c0.txt");
        if (!is) {
            log_error << "Open error";
            return -1;
        }

        auto rc = ipr.load(is);
        if (rc)
            return 1;

//        ifstream ipdata("test/iplog.dat");
//        ipr.processData(ipdata);

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

#ifndef UT_CATCH
int main()
{
    IPRegistry ipr;
    auto rc = init(ipr);

//    ofstream os("report_30.txt");
//    ipr.report(os);

    return rc;
}
#endif // CATCH_CONFIG_MAIN


#ifdef UT_CATCH
#include "catch.hpp"

/*
TEST_CASE( "IP4Address.lookup", "[IP4Address]")
{
    Tree tree;
    init(tree);
    IP4Address ip("239.254.0.0");
    Node* p1 = tree.lookup(ip);
    auto correct = ( (p1->iaddr == (239<<24) + (254<<16)) && p1->bits == 15);
    REQUIRE( correct );
}
*/

#endif // UT_CATCH
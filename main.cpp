#include <iostream>
#include <fstream>
#include <string>
#include <memory> // make_unique

#include <arpa/inet.h>

#include "trace.h"
#include "subnets.h"

using namespace std;

struct RegElem {
    RegElem(const string& name, const string& addr, int bits)
        : id(name), netw(std::make_unique<IPData>(addr, bits)) {}

    string id;
    std::unique_ptr<IPData> netw;
};

#include <vector>
using IPRegistry = std::vector<RegElem>;

// Load registry
int load(istream& is, IPRegistry& ipr)
{
    string id; // customer id
    string netw; // text repr of the network
    int bits;
    // use getline to validate input format
    int lines = 0;
    while (is >> id >> netw) {
        ++lines;
        bits = 32;
        try {
            auto pos = netw.find('/');
            if (pos != string::npos) {
                bits = strtol(netw.c_str() + pos + 1, 0, 10); /// @todo check format: any extra symbols
                if (bits == 0)
                    throw std::invalid_argument(netw);
                netw.erase(pos);
            }
            ipr.push_back({id, netw, bits});
        //  log_trace << lines << ": " << netw << "|" << bits;
        }
        catch (std::exception& e) {
            log_error << "Bad format: line " << lines << " " << e.what();
            is.setstate(std::ios_base::badbit);
        //    break;
        }

    }
    log_info << "Done " << lines << " lines";
    // check eof and failbit. Stop ib case of failbit but not eof, as it means the stream is heavily misformatted
    return lines;
}

int init()
{
    try {
        ifstream is;
        is.open("test/c5.log");
        if (!is) {
            log_error << "Open error";
            return -1;
        }

        IPRegistry ipr;
        load(is, ipr);
        if (is.bad())
            return 1;

        Tree tree;
        vector<Node> nodes;

        for (auto& x : ipr) {
        //  log_trace << x.id << '\t' << *x.netw;
            Node* node = new Node(x.netw.get()); /// @todo leak
            tree.insert(node);
        }

        log_trace << "Tree:\n" << tree;

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

#ifndef CATCH_CONFIG_MAIN
int main()
{
    auto rc = init();

//    char s[] = "192.168.0.5";
//    in_addr   addr;
//    unsigned char* a = (unsigned char*)&addr.s_addr;
//    auto rc =  inet_pton(AF_INET, s, &addr);
//    cout << rc << endl;
//    printf("%u.%u.%u.%u\n", a[0], a[1], a[2], a[3]);

    return rc;
}
#endif // CATCH_CONFIG_MAIN

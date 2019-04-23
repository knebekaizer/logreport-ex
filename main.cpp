#include <iostream>
#include <fstream>
#include <string>

#include <arpa/inet.h>

#include "trace.h"

using namespace std;

struct IP4Network {
    in_addr_t   addr_;
    uint8_t     bits;
};

struct RegElem {
    string id;
    in_addr addr;

};

// Load registry
int load(istream& is)
{
    string id; // customer id
    string netw; // text repr of the network
    int bits;
    // use getline to validate input format
    int lines = 0;
    while (is >> id >> netw) {
    //    log_trace << id << " - " << netw;
        auto pos = netw.find('/');
        if (pos != string::npos) {
            bits = strtol(netw.c_str() + pos + 1, 0, 10);
            if (bits == 0) {
                log_error << "Format error";
            }
            netw.erase(pos);
        }
       tr_stream << netw << "|" << bits;

        ++lines;
    }
    log_info << "Done " << lines << " lines";
    // check eof and failbit. Stop ib case of failbit but not eof, as it means the stream is heavily misformatted
    return 0;
}

int main() {
    std::cout << "Hello, World!" << std::endl;

    ifstream is;
    is.open("c1.log");
    if (!is) cout << "Open error" << endl;
    load(is);
//    char s[] = "192.168.0.5";
//    in_addr   addr;
//    unsigned char* a = (unsigned char*)&addr.s_addr;
//    auto rc =  inet_pton(AF_INET, s, &addr);
//    cout << rc << endl;
//    printf("%u.%u.%u.%u\n", a[0], a[1], a[2], a[3]);

    return 0;
}

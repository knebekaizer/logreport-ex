/// @file

#include <iostream>

using namespace std;

class Parser;

// Use struct to say that there is no invariants enforced
struct LogEntry {
    typedef uint64_t CountT;
    IpKey   ip;
    CountT  bytes;
};

int log_read(istream& is)
{
    LogEntry line;


    // use getline to validate input format
    while (is >> line) {
        registry.findByIp(line.ip).add(bytes); // never fail
    }

    // check eof and failbit. Stop ib case of failbit but not eof, as it means the stream is heavily misformatted
}
/// @file

#include <iostream>

using namespace std;

class Parser;

// container that maps ip range to byte counter (accumulator)
// Can traverse (seek for the most specific subnet including particular ip)
class Subnets;

// container that maps customer id to the list of owned subnets
// To get the report summary, one shall sum numbers for all owned subnets
class Customers;

class Customers {
    // for each entry add the subnet to one of two Subnets (ipv4 or ipv6) and link it to
    // existing or newly created customer
    int load(istream& is);
};

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
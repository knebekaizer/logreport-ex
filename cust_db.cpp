//! @file

#include "cust_db.h"

#include <iostream>
#include <array>
#include <cstdint>

using namespace std;

class Subnet;
class SubnetsData;
class CustomerID;
class Customers;

// immutable object once created (aggregate)
class Subnet {
    using ipv4_t = std::array<uint8_t, 4>;

    ipv4_t  ip() const;
    ipv4_t  mask() const;
private:
    const ipv4_t ip_;
    const ipv4_t mask_;
};

class SubnetsData {
    void resetData();
};

class CustDB {
public:
    int load(istream& is);
    void walk(std::function<void<const Node&>);

private:
    Customers   customers;
    Subnets     subnets;
};

int
CustDB::load(istream& is)
{
    Subnet net;
    CustomerID customerId;

    while (is >> customerId >> net) {
        auto node = subnets.insert(net);
        customers.insert(customerId, node);
    }
}

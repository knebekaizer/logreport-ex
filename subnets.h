/// @file

#ifndef IPLOG_TEST_SUBNETS_H
#define IPLOG_TEST_SUBNETS_H

#endif //IPLOG_TEST_SUBNETS_H

#include <cstdint>
#include <set>


class IP {
public:
    using AddrT = uint64_t;
    using MaskT = uint64_t;
    AddrT   addr() const { return addr_; }
    MaskT   mask() const { return mask_; }
private:
    AddrT   addr_ = 0;
    MaskT   mask_ = ~0;
};



class Node : public IP {
public:
    friend class Tree;
    using CountT = uint64_t; // need more bits?
    CountT  data() const { return data_; }
    CountT  addData(uint64_t incr) { data_ += incr; } /// @todo Overflow?

    bool    contains(const Node* sub) const;
private:
    CountT  data_ = 0;
    struct less : std::binary_function<Node*, Node*, bool> {
        bool operator()(const Node* x, const Node* y) const { return x->addr() < y->addr(); }
    };
    std::set<Node*, Node::less>  subs;
};

class Tree : public Node {
public:
    void insert(Node* ip);
};

bool Node::contains(const Node* sub) const
{
    // widest mask is numerically less than the narrow one
    return (mask() < sub->mask()
            && (sub->addr() & mask()) == addr());
}

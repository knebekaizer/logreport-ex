//
// Created by Vladimir Ivanov on 2019-04-23.
//

#include "subnets.h"

#include <set>

// tree
void Tree::insert(Node* ip)
{
    // Root owns all:
    // When building, the root is just a list of top-level nets
    // When accumelating, root.add means "unknown"

    Node* current = this;
    auto bound = current->subs.end();
    while (!current->subs.empty()) {
        // for current level of nesting
        bound = std::lower_bound(current->subs.begin(), current->subs.end(), ip);
        if (bound == current->subs.begin()) {
            // not found in children, insert into the current level
            break;
        } else if ((*bound)->addr() == ip->addr()) {
            // exact match or subnet?
            if ((*bound)->mask() == ip->mask()) {
                // Check for exact match when building. Ignore (warn) if owner is the same, otherwise raise error
                // Recurse deeper when accumulating. Do nothing here, it's implemented below
            } else if ((*bound)->mask() < ip->mask()) {
                assert((*bound)->contains(ip));
                current = *bound;
                continue;
            } else {
                assert(ip->contains(*bound));
                break;
            }
        } else {
            // one step back over sorted siblings
            auto prev = std::prev(bound);
            if ((*prev)->contains(ip)) {
                // continue recursion
                current = *prev;
                continue;
            } else {
                break;
            }
        }
    }

    for (auto it = bound; it != current->subs.end() && ip->contains(*it); ++it) {
        // move subs from the current to the new one
        ip->subs.insert(*it);
        current->subs.erase(it);
    }
    current->subs.insert(ip); // use bound as a hint
}

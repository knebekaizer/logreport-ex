// radix
function lookup(string x)
{
    // Begin at the root with no elements found
    Node traverseNode := root;
    int elementsFound := 0;

    // Traverse until a leaf is found or it is not possible to continue
    while (traverseNode != null && !traverseNode.isLeaf() && elementsFound < x.length)
    {
        // Get the next edge to explore based on the elements not yet found in x
        Edge nextEdge := select edge from traverseNode.edges where edge.label is a prefix of x.suffix(elementsFound)
        // x.suffix(elementsFound) returns the last (x.length - elementsFound) elements of x

        // Was an edge found?
        if (nextEdge != null)
        {
            // Set the next node to explore
            traverseNode := nextEdge.targetNode;

            // Increment elements found based on the label stored at the edge
            elementsFound += nextEdge.label.length;
        }
        else
        {
            // Terminate loop
            traverseNode := null;
        }
    }

    // A match is found if we arrive at a leaf node and have used up exactly x.length elements
    return (traverseNode != null && traverseNode.isLeaf() && elementsFound == x.length);
}

using namespace std;

// tree
Node lookup(Node root, IP ip)
{
    // Root owns all:
    // When building, the root is just a list of top-level nets
    // When accumelating, root.add means "unknown"

    Node current = root;
    while (!current.children.empty()) {
        // for current level of nesting
        auto it = std::lower_bound(children.begin(), children.end(), t);
        if (it == children.begin()) {
            // not found in the children, stay on the current level
            break;
        } else {
            if (it->addr == ip.addr) {
                // exact match or subnet?
                // Check for exact match when building. Ignore (warn) if owner is the same, otherwise raise error
                // Recurse deeper when accumulating. Do nothing here, it's implemented below
            } else {
                // one step back over sorted siblings
                --it;
            }
            if (ip.isSubOf(*it)) {
                // continue recursion
                current = *it;
                continue;
            } else {
                break;
            }
        }
        return current;
        // insert as a new child when building or add bytes to the current when accumulating
        current.add(ip); // or children.insert(ip)

    }
}

function accumulate(IP ip, CountT bytes) {
    auto node = lookup(root, ip);
    node.add(bytes);
}

function insert(IP ip) {
    auto node = lookup(root, ip);
    node.insertAndRebuild(ip, hint); // check if some children are subs of new one and move them accordingly
}

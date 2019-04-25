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


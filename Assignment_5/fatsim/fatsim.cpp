
#include "fatsim.h"
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;

vector<pair<long, int>> leaf_nodes;
long max_depth = 1;

// traverse through all parents who have children
void BFS(long int parent, const vector<vector<long>> &adj_list, vector<long> &nodes) {
    queue<long> new_parent;
    new_parent.push(parent);

    while(!new_parent.empty()) {
        parent = new_parent.front();
        new_parent.pop();

        // visit all children of the new parent
        for (auto &child : adj_list[parent]) {
            long depth = nodes[parent] + 1;
            nodes[child] = depth;
            if (depth > max_depth) {
                max_depth = depth;
            }

            // leaf node
            if (adj_list[child].empty()) {
                leaf_nodes.push_back(make_pair(child, depth));
            }
            else {  
                new_parent.push(child);       // push the child to queue if it also has children
            }
        }
    }
}

bool sort_depth(const pair<int,int> &a, const pair<int,int> &b) {
    return (a.second > b.second);
}

// reimplement this function
vector<long> fat_check(const vector<long> &fat)
{
    // this vector will contain information about all nodes in the graph
    // - the node, its count and if it has been visited
    vector<long> nodes(fat.size()+1);

    vector<vector<long>> adj_list(fat.size()+1);    // for graph
    vector<long> longest_parent;                    // contains the nodes with the largest depth

    // adding to adj_list, mapped all nodes to "+ 1" so I can keep track of nodes at -1 
    for (int i = 0; i < int(fat.size()); i++) {
        long new_ind = i+1;
        long val = fat[i];
        long new_val = val+1;
        adj_list[new_val].push_back(new_ind);
    }

    // iterate from nodes that point FROM -1 (which is 0) 
    for (auto &parent : adj_list[0]) {        
        nodes[parent] = 1;                    // mark the first parent distance as 1

        // parents are leaf nodes themselves, this will take care of cases when graph have max_depth of 1
        if (adj_list[parent].empty()) {
            leaf_nodes.push_back(make_pair(parent, 1));
        }
        // traverse through every child of the parent using iterative BFS
        else {
            BFS(parent, adj_list, nodes);   
        }
    }

    // sort by the depth of the leaf nodes
    sort(leaf_nodes.begin(), leaf_nodes.end(), sort_depth);  

    // go through each leaf node and find the ones with the max depth
    for (auto &leaf : leaf_nodes) {
        if (nodes[leaf.first] == max_depth) {
            longest_parent.push_back(leaf.first-1);     // re-map the original node by subtracting 1
        }
        else { break; }                                 // break since the leaves are sorted by depth we don't need to "look" anymore
    }
    // sort the results
    sort(longest_parent.begin(), longest_parent.end());
    
    return longest_parent;
}


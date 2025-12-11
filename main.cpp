#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
using namespace std;

const int M = 5;
const int NODE_SIZE = 1 + M * 2;
const int NUM_NODES = 10;


struct LeafSplitResult {
    int leftIndex;
    int rightIndex;
    int maxLeft;
    int maxRight;
};
struct InternalSplitResult {
    int leftIndex;
    int rightIndex;
    int promotedKey;
};


LeafSplitResult SplitLeaf(fstream &file,
                          int leafIndex,
                          vector<int> &leaf,
                          int newKey,
                          int newRef,
                          bool isRootSplit);

InternalSplitResult SplitInternalNode(fstream& file,
                                      int nodeIndex,
                                      vector<int>& node,
                                      int newKey,
                                      int newChild
                                    );

void ApplyLeafSplitToParent(fstream &file,
                            int parentIdx,
                            const LeafSplitResult &split);

void ApplyInternalSplitToParent(fstream& file,
                                int parentIdx,
                                const InternalSplitResult& split);

int FindParentIndex(fstream &file, int childIndex);
void RefreshParentMaxKeyForLeaf(fstream &file, int leafIndex);

void ReadNode(fstream &file, int index, vector<int> &node) {
    node.resize(NODE_SIZE);
    file.seekg(index * NODE_SIZE * sizeof(int), ios::beg);
    file.read(reinterpret_cast<char*>(node.data()), NODE_SIZE * sizeof(int));
}

void WriteNode(fstream &file, int index, const vector<int> &node) {

    file.seekp(index * NODE_SIZE * sizeof(int), ios::beg);
    file.write(reinterpret_cast<const char*>(node.data()), NODE_SIZE * sizeof(int));
}

   //ALLOCATE NODE FROM FREE LIST
int AllocateNode(fstream &file, int leafFlag) {

    vector<int> header(NODE_SIZE);
    ReadNode(file, 0, header);

    int freeIndex = header[1];
    if (freeIndex == -1) {
        return -1;}

    // Read next free node
    vector<int> freeNode(NODE_SIZE);
    ReadNode(file, freeIndex, freeNode);
    int nextFree = freeNode[1];

    // Update header
    header[1] = nextFree;
    WriteNode(file, 0, header);

    // Initialize new node
    vector<int> node(NODE_SIZE, -1);
    node[0] = leafFlag;
    WriteNode(file, freeIndex, node);

    return freeIndex;
}

   //INSERT INTO LEAF

bool InsertIntoLeaf(vector<int> &leaf, int key, int ref) {
    // Count existing items
    int count = 0;
    for (int i = 0; i < M; i++)
        if (leaf[1 + i*2] != -1) count++;

    // Leaf full → split required
    if (count == M)
        return false;

    // Insert into first free slot
    for (int i = 0; i < M; i++) {
        if (leaf[1 + i*2] == -1) {
            leaf[1 + i*2]     = key;
            leaf[1 + i*2 + 1] = ref;
            break;
        }
    }

    vector<pair<int,int>> items;
    for (int i = 0; i < M; i++) {
        int k = leaf[1 + i*2];
        int r = leaf[1 + i*2 + 1];
        if (k != -1) items.push_back({k, r});
    }
    sort(items.begin(), items.end());

    // Rewrite leaf
    int pos = 0;
    for (; pos < items.size(); pos++) {
        leaf[1 + pos*2]     = items[pos].first;
        leaf[1 + pos*2 + 1] = items[pos].second;
    }
    for (; pos < M; pos++) {
        leaf[1 + pos*2] = leaf[1 + pos*2 + 1] = -1;
    }

    return true;
}

int ComputeInternalMaxKey(fstream &file, int nodeIndex)
{
    vector<int> node(NODE_SIZE);
    ReadNode(file, nodeIndex, node);

    int maxKey = -1;
    for (int i = 0; i < M; i++) {
        int k = node[1 + i*2];
        if (k != -1) maxKey = k;
    }
    return maxKey;
}

// Find the index of the parent INTERNAL node that has a child == childIndex
int FindParentIndex(fstream &file, int childIndex) {
    vector<int> node(NODE_SIZE);
    for (int i = 1; i < NUM_NODES; i++) {
        ReadNode(file, i, node);
        if (node[0] != 1) continue;

        for (int j = 0; j < M; j++) {
            int c = node[1 + j*2 + 1];
            if (c == childIndex) {
                return i;
            }
        }
    }
    return -1;
}

// Recompute the max key of a LEAF and update the parent's entry for it
void RefreshParentMaxKeyForLeaf(fstream &file, int leafIndex) {
    // Compute leaf max
    vector<int> leaf(NODE_SIZE);
    ReadNode(file, leafIndex, leaf);
    if (leaf[0] != 0) return; // not a leaf

    int newMax = -1;
    for (int i = 0; i < M; i++) {
        int k = leaf[1 + i*2];
        if (k != -1) newMax = k;
    }
    if (newMax == -1) return; // empty leaf

    int parentIdx = FindParentIndex(file, leafIndex);
    if (parentIdx == -1) return; // root or no parent

    vector<int> parent(NODE_SIZE);
    ReadNode(file, parentIdx, parent);

    // Collect all existing (key,child) pairs, replacing the one for this leaf
    vector<pair<int,int>> entries;
    for (int i = 0; i < M; i++) {
        int k = parent[1 + i*2];
        int c = parent[1 + i*2 + 1];
        if (k == -1) continue;

        if (c == leafIndex) {
            entries.push_back({newMax, leafIndex});
        } else {
            entries.push_back({k, c});
        }
    }

    sort(entries.begin(), entries.end(),
         [](const pair<int,int> &a, const pair<int,int> &b) {
             return a.first < b.first;
         });

    // Rewrite parent node
    for (int i = 0; i < NODE_SIZE; i++) parent[i] = -1;
    parent[0] = 1; // internal

    for (int i = 0; i < (int)entries.size() && i < M; i++) {
        parent[1 + i*2]     = entries[i].first;
        parent[1 + i*2 + 1] = entries[i].second;
    }

    WriteNode(file, parentIdx, parent);
}

// After a NON-ROOT leaf split, update the parent
void ApplyLeafSplitToParent(fstream &file,
                            int parentIdx,
                            const LeafSplitResult &split)
{
    vector<int> parent(NODE_SIZE);
    ReadNode(file, parentIdx, parent);

    vector<pair<int,int>> entries;

    bool replaced = false;
    for (int i = 0; i < M; i++) {
        int k = parent[1 + i*2];
        int c = parent[1 + i*2 + 1];
        if (k == -1) continue;

        if (!replaced && c == split.leftIndex) {
            entries.push_back({split.maxLeft,  split.leftIndex});
            entries.push_back({split.maxRight, split.rightIndex});
            replaced = true;
        } else {
            entries.push_back({k, c});
        }
    }
    sort(entries.begin(), entries.end());
    // If PARENT overflows → MUST SPLIT INTERNAL NODE
    if (entries.size() > M) {
        vector<int> tempParent = parent;
        for (int i = 0; i < NODE_SIZE; i++) tempParent[i] = -1;
        tempParent[0] = 1;

        for (int i = 0; i < M; i++) {
            tempParent[1 + i*2]     = entries[i].first;
            tempParent[1 + i*2 + 1] = entries[i].second;
        }
        WriteNode(file, parentIdx, tempParent);

        int newKey = split.maxRight;
        int newChild = split.rightIndex;
        InternalSplitResult pSplit =
            SplitInternalNode(file, parentIdx, tempParent,newKey, newChild);

        int grandParent = FindParentIndex(file, parentIdx);
        if (grandParent == -1) {
            // parent was ROOT
            vector<int> newRoot(NODE_SIZE, -1);
            newRoot[0] = 1;
            int maxLeft  = ComputeInternalMaxKey(file, pSplit.leftIndex);
            int maxRight = ComputeInternalMaxKey(file, pSplit.rightIndex);

            newRoot[1] = pSplit.promotedKey;
            newRoot[2] = pSplit.leftIndex;
            newRoot[3] = maxRight;
            newRoot[4] = pSplit.rightIndex;

            WriteNode(file, 1, newRoot);
            return;
        }
        ApplyInternalSplitToParent(file, grandParent, pSplit);
        return;
    }
    // Parent fits -> rewrite
    vector<int> newParent(NODE_SIZE, -1);
    newParent[0] = 1;

    for (int i = 0; i < entries.size(); i++) {
        newParent[1 + i*2]     = entries[i].first;
        newParent[1 + i*2 + 1] = entries[i].second;
    }

    WriteNode(file, parentIdx, newParent);
}

void ApplyInternalSplitToParent(fstream& file,
                                int parentIdx,
                                const InternalSplitResult& split)
{
    vector<int> parent(NODE_SIZE);
    ReadNode(file, parentIdx, parent);

    vector<pair<int,int>> entries;
    // Insert existing entries
    for (int i = 0; i < M; i++) {
        int k = parent[1 + i*2];
        int c = parent[1 + i*2 + 1];
        if (k != -1)
            entries.push_back({k, c});
    }
    // Insert the two new internal children
    int maxLeft  = ComputeInternalMaxKey(file, split.leftIndex);
    int maxRight = ComputeInternalMaxKey(file, split.rightIndex);

    entries.push_back({maxLeft,  split.leftIndex});
    entries.push_back({maxRight, split.rightIndex});

    sort(entries.begin(), entries.end(),
         [](auto& a, auto& b){ return a.first < b.first; });

    // If parent overflows → must split PARENT
    if (entries.size() > M) {
        vector<int> nodeParent = parent;
        for (int i = 0; i < NODE_SIZE; i++) nodeParent[i] = -1;
        nodeParent[0] = 1;

        // Write temporary truncated version (required before split)
        for (int i = 0; i < M; i++) {
            nodeParent[1 + i*2]     = entries[i].first;
            nodeParent[1 + i*2 + 1] = entries[i].second;
        }
        WriteNode(file, parentIdx, nodeParent);
        int newKey   = maxRight;
        int newChild = split.rightIndex;

        InternalSplitResult pSplit =
            SplitInternalNode(file, parentIdx, nodeParent,newKey, newChild);

        int grandParent = FindParentIndex(file, parentIdx);
        if (grandParent == -1) {
            // parent was ROOT → must create NEW root internal node
            vector<int> newRoot(NODE_SIZE, -1);
            newRoot[0] = 1;

            newRoot[1] = pSplit.promotedKey;
            newRoot[2] = pSplit.leftIndex;
            newRoot[3] = maxRight;
            newRoot[4] = pSplit.rightIndex;

            WriteNode(file, 1, newRoot);
            return;
        }

        ApplyInternalSplitToParent(file, grandParent, pSplit);
        return;
    }

    vector<int> newParent(NODE_SIZE, -1);
    newParent[0] = 1;

    for (int i = 0; i < entries.size(); i++) {
        newParent[1 + i*2]     = entries[i].first;
        newParent[1 + i*2 + 1] = entries[i].second;
    }

    WriteNode(file, parentIdx, newParent);
}

LeafSplitResult SplitLeaf(fstream &file,
                          int leafIndex,
                          vector<int> &leaf,
                          int newKey,
                          int newRef,
                          bool isRootSplit)
{
    // 1) Collect existing items
    vector<pair<int,int>> items;
    for (int i = 0; i < M; i++) {
        int key = leaf[1 + i*2];
        int ref = leaf[1 + i*2 + 1];
        if (key != -1)
            items.push_back({key, ref});
    }

    // 2) Add the new item that caused the split
    items.push_back({newKey, newRef});
    // 3) Sort all keys
    sort(items.begin(), items.end());
    // 4) Median rule from assignment: median = (n-1)/2
    int median = (int(items.size()) - 1) / 2;

    vector<pair<int,int>> leftItems(items.begin(), items.begin() + median + 1);
    vector<pair<int,int>> rightItems(items.begin() + median + 1, items.end());

    int leftIndex, rightIndex;

    if (isRootSplit) {
        // Root was a leaf → allocate TWO brand new leaves
        leftIndex  = AllocateNode(file, 0);
        rightIndex = AllocateNode(file, 0);
    } else {
        // Non-root leaf split → reuse the same index for LEFT child,
        // and allocate a new index for the RIGHT child
        leftIndex  = leafIndex;
        rightIndex = AllocateNode(file, 0);
    }

    // 5) Build LEFT leaf
    vector<int> leftNode(NODE_SIZE, -1);
    leftNode[0] = 0;  // leaf flag
    for (int i = 0; i < (int)leftItems.size(); i++) {
        leftNode[1 + i*2]     = leftItems[i].first;
        leftNode[1 + i*2 + 1] = leftItems[i].second;
    }
    WriteNode(file, leftIndex, leftNode);

    // 6) Build RIGHT leaf
    vector<int> rightNode(NODE_SIZE, -1);
    rightNode[0] = 0;
    for (int i = 0; i < (int)rightItems.size(); i++) {
        rightNode[1 + i*2]     = rightItems[i].first;
        rightNode[1 + i*2 + 1] = rightItems[i].second;
    }
    WriteNode(file, rightIndex, rightNode);

    LeafSplitResult result;
    result.leftIndex  = leftIndex;
    result.rightIndex = rightIndex;
    result.maxLeft    = leftItems.back().first;
    result.maxRight   = rightItems.back().first;

    return result;
}

InternalSplitResult SplitInternalNode(fstream& file,
                                      int nodeIndex,
                                      vector<int>& node,
                                      int newKey,
                                      int newChild
                                    )
{

    // Collect (key, child) entries
    vector<pair<int,int>> items;
    for (int i = 0; i < M; i++) {
        int k = node[1 + i*2];
        int c = node[1 + i*2 + 1];
        if (k != -1)
            items.push_back({k, c});
    }
    items.push_back({newKey, newChild});

    sort(items.begin(), items.end());

    int median = (items.size() - 1) / 2;
    int promotedKey = items[median].first;
    int promotedChild = items[median].second;

    vector<pair<int,int>> leftItems(items.begin(), items.begin() + median);
    vector<pair<int,int>> rightItems(items.begin() + median + 1, items.end());
    leftItems.insert(leftItems.end(), {promotedKey, promotedChild});

    int leftIndex;
    int rightIndex ;
    bool isRoot = (nodeIndex == 1);

    if (isRoot) {
        leftIndex  = AllocateNode(file, 1);
        rightIndex = AllocateNode(file, 1);
    } else {
        leftIndex  = nodeIndex;               // NEW LEFT LINE
        rightIndex = AllocateNode(file, 1);   // NEW RIGHT LINE
    }



    // Build LEFT node
    vector<int> leftNode(NODE_SIZE, -1);
    leftNode[0] = 1;
    for (int i = 0; i < leftItems.size(); i++) {
        leftNode[1 + i*2]     = leftItems[i].first;
        leftNode[1 + i*2 + 1] = leftItems[i].second;
    }
    WriteNode(file, leftIndex, leftNode);

    // Build RIGHT node
    vector<int> rightNode(NODE_SIZE, -1);
    rightNode[0] = 1;
    for (int i = 0; i < rightItems.size(); i++) {
        rightNode[1 + i*2]     = rightItems[i].first;
        rightNode[1 + i*2 + 1] = rightItems[i].second;
    }
    WriteNode(file, rightIndex, rightNode);

    InternalSplitResult r;
    r.leftIndex   = leftIndex;
    r.rightIndex  = rightIndex;
    r.promotedKey = promotedKey;

    return r;
}

bool InsertIntoInternal(vector<int> &node, int key, int childIndex) {

    // Full?
    if (node[1 + (M-1)*2] != -1)
        return false;

    int pos = 0;
    while (pos < M && node[1 + pos*2] != -1 && node[1 + pos*2] < key)
        pos++;

    for (int i = M-1; i > pos; i--) {
        node[1 + i*2]     = node[1 + (i-1)*2];
        node[1 + i*2 + 1] = node[1 + (i-1)*2 + 1];
    }

    node[1 + pos*2]     = key;
    node[1 + pos*2 + 1] = childIndex;

    return true;
}

/* =======================================================================
   PICK CHILD BASED ON MAX-KEY RULE
   ======================================================================= */

int PickChild(const vector<int> &node, int key) {

    int lastValidChild = -1;

    for (int i = 0; i < M; i++) {

        int k = node[1 + i*2];
        int c = node[1 + i*2 + 1];

        if (c != -1) lastValidChild = c;

        if (k == -1)
            return lastValidChild;

        if (key <= k)
            return c;
    }

    return lastValidChild;
}

int InsertNewRecordAtIndex(char* filename, int key, int ref) {

    fstream file(filename, ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "Error opening index file.\n";
        return -1;
    }

    int current = 1;
    vector<int> node;

    // Ensure root exists, otherwise create leaf at node 1
    ReadNode(file, 1, node);
    if (node[0] == -1) {

        vector<int> header(NODE_SIZE);
        ReadNode(file, 0, header);

        int freeIndex = header[1];
        int nextFree;

        {
            vector<int> tmp(NODE_SIZE);
            ReadNode(file, freeIndex, tmp);
            nextFree = tmp[1];
        }

        header[1] = nextFree;      // remove node 1 from free list
        WriteNode(file, 0, header);

        vector<int> newRoot(NODE_SIZE, -1);
        newRoot[0] = 0;            // leaf
        WriteNode(file, 1, newRoot);
    }

    while (true) {
        ReadNode(file, current, node);

        if (node[0] == 0) {
            // LEAF
            if (InsertIntoLeaf(node, key, ref)) {
                WriteNode(file, current, node);

                // If this is not the root, parent may need new max key
                if (current != 1)
                    RefreshParentMaxKeyForLeaf(file, current);
                return current;
            }

            // Split leaf
            bool isRootSplit = (current == 1 && node[0] == 0);

            LeafSplitResult split = SplitLeaf(file, current, node, key, ref, isRootSplit);
            if (isRootSplit) {
                // ROOT WAS LEAF → now becomes INTERNAL at node 1

                vector<int> root(NODE_SIZE, -1);
                root[0] = 1; // internal

                root[1] = split.maxLeft;
                root[2] = split.leftIndex;

                root[3] = split.maxRight;
                root[4] = split.rightIndex;

                WriteNode(file, 1, root);
                return 1;
            } else {
                // ---------------------------------------------------
                // NON-ROOT LEAF SPLIT → update parent internal node
                // ---------------------------------------------------
                int parentIdx = FindParentIndex(file, current);
                if (parentIdx != -1) {
                    ApplyLeafSplitToParent(file, parentIdx, split);
                } else {
                    cout << "[WARN] No parent found for split leaf " << current
                         << " (this should not happen except for root)\n";
                }

                return current;
            }
        }

        // -----------------------------------------------------------
        // INTERNAL NODE → pick child and descend
        // -----------------------------------------------------------

        int next = PickChild(node, key);

        if (next == -1) {
            cerr << "[ERROR] PickChild returned -1 (no valid child). Aborting.\n";
            return -1;
        }

        current = next;
    }
}

void CreateIndexFileFile(char* filename, int numberOfNodes, int m) {
    ofstream file(filename, ios::binary | ios::out);

    vector<int> node(NODE_SIZE, -1);

    node[0] = -1;
    node[1] = 1;  // free list begins at node 1
    file.write(reinterpret_cast<char*>(node.data()), NODE_SIZE*sizeof(int));

    for (int i = 1; i < numberOfNodes; i++) {
        node[0] = -1;
        node[1] = (i == numberOfNodes - 1 ? -1 : i+1);
        for (int j = 2; j < NODE_SIZE; j++)
            node[j] = -1;

        file.write(reinterpret_cast<char*>(node.data()), NODE_SIZE*sizeof(int));
    }
}

void DisplayIndexFileContent(char* filename) {
    ifstream file(filename, ios::binary);

    vector<int> node(NODE_SIZE);
    int index = 0;

    cout << "\n===== INDEX FILE CONTENT =====\n";
    while (file.read(reinterpret_cast<char*>(node.data()), NODE_SIZE*sizeof(int))) {
        cout << "Node " << index << ": ";
        for (int v : node) cout << v << " ";
        cout << "\n";
        index++;
    }
    cout << "==============================\n";
}
// ====================  Search==============
int SearchARecord(char* filename, int RecordID) {

    fstream file(filename, ios::in | ios::binary);
    if (!file.is_open()) {
        cout << "error while opening file ";
        return -1;
    }

    int nodeIndex = 1;
    vector<int> node(NODE_SIZE);

    while (true) {
        file.seekg(nodeIndex * NODE_SIZE * sizeof(int), ios::beg);
        file.read(reinterpret_cast<char*>(node.data()), NODE_SIZE * sizeof(int));
        if (!file) return -1;

        if (node[0] == 0) {
            for (int i = 0; i < M; ++i) {
                int keyIdx = 1 + i*2;
                int refIdx = keyIdx + 1;
                if (keyIdx < NODE_SIZE && node[keyIdx] == RecordID) {
                    if (refIdx < NODE_SIZE) return node[refIdx];
                    else return -1;
                }
            }
            return -1;
        }
        int nextChild = -1;
        int lastValidChild = -1;
        for (int i = 0; i < M; ++i) {
            int k = (1 + i*2 < NODE_SIZE) ? node[1 + i*2] : -1;
            int c = (1 + i*2 + 1 < NODE_SIZE) ? node[1 + i*2 + 1] : -1;
            if (c != -1) lastValidChild = c;
            if (k == -1) {
                nextChild = lastValidChild;
                break;
            }
            if (RecordID <= k) {
                nextChild = c;
                break;
            }
        }
        if (nextChild == -1) nextChild = lastValidChild;
        if (nextChild == -1) return -1;
        nodeIndex = nextChild;
    }

    return -1;
}
//================= Delete From Zero =============
void RefreshParentMaxKeyForLeafAbdo(fstream &file, int leafIndex, int oldLeafMax = -1) {
    vector<int> leaf(NODE_SIZE);
    ReadNode(file, leafIndex, leaf);

    if (leaf[0] != 0){
        return; // not a leaf
    }
    
    // compute newMax for the leaf (after deletion)
    int newMax = -1;
    for (int i = 0; i < M; ++i) {
        int k = leaf[1 + i*2];
        if (k != -1) {
            newMax = k;
        }
    }
    // If leaf became empty, we still attempt to update parent (newMax == -1)
    int childIdx = leafIndex;
    int parentIdx = FindParentIndex(file, childIdx);
    if (parentIdx == -1){ 
        return;  // no parent -> done
    }

    // We'll propagate upwards, carrying the updated max for the child we just fixed.
    int childMax = newMax;

    while (parentIdx != -1) {
        vector<int> parent(NODE_SIZE);
        ReadNode(file, parentIdx, parent);

        // Build list of entries (key,child) present in this parent
        vector<pair<int,int>> entries;
        entries.reserve(M);
        for (int i = 0; i < M; ++i) {
            int k = parent[1 + i*2];
            int c = parent[1 + i*2 + 1];
            if (k != -1) entries.emplace_back(k, c);
        }

        if (entries.empty()) {
            // nothing to do for this parent
            break;
        }

        // Try to find which entry corresponds to our child
        bool replaced = false;

        // match by exact child index (works if parent stores correct child pointers)
        for (auto &e : entries) {
            if (e.second == childIdx) {
                // replace the key with new child max (childMax may be -1 if child empty)
                e.first = childMax;
                replaced = true;
                break;
            }
        }

        // If we still didn't replace anything, nothing sensible to do -> stop
        if (!replaced) break;

        // Sort entries by key ascending (to keep your internal node order)
        sort(entries.begin(), entries.end(), [](const pair<int,int>& a, const pair<int,int>& b){
            return a.first < b.first;
        });

        // Compute new parent max (highest key among entries) for propagation
        int newParentMax = entries.back().first;

        // Check if parent actually changes to avoid unnecessary writes/propagation:
        int oldParentMax = -1;
        for (auto &e : entries) {
            oldParentMax = max(oldParentMax, e.first);
        } 
        // Rewrite parent node array from entries (clear first)
        for (int i = 0; i < NODE_SIZE; ++i){ 
            parent[i] = -1;
        }
        parent[0] = 1; // internal

        for (int i = 0; i < (int)entries.size() && i < M; ++i) {
            parent[1 + i*2]     = entries[i].first;
            parent[1 + i*2 + 1] = entries[i].second;
        }

        // Write updated parent back
        WriteNode(file, parentIdx, parent);

        // Prepare to move up: the parent's max changed to newParentMax (that's what ancestors store for this parent)
        // If the parent's stored key that refers to it in its parent was previously the same as newParentMax, we can stop.
        // But we must read grandparent to check. So move upwards:
        childIdx = parentIdx;
        childMax = newParentMax;
        oldLeafMax = -1; // we used oldLeafMax (if present) only for the immediate parent match
        parentIdx = FindParentIndex(file, childIdx);
    }
}

void RemovePairAtPosition(fstream &file, int nodeIndex, int pos) {
    vector<int> node(NODE_SIZE);
    ReadNode(file, nodeIndex, node);

    if (pos < 0 || pos >= M) return;

    for (int i = pos; i < M - 1; ++i) {
        node[1 + i*2] = node[1 + (i+1)*2];
        node[1 + i*2 + 1] = node[1 + (i+1)*2 + 1];
    }
    node[1 + (M-1)*2]   = -1;
    node[1 + (M-1)*2 + 1] = -1;
    WriteNode(file, nodeIndex, node);
}

int FindChildSlotInParent(fstream &file, int parentIdx, int childIdx) {
    vector<int> parent(NODE_SIZE);
    ReadNode(file, parentIdx, parent);

    for (int i = 0; i < M; ++i) {
        int keyPos = 1 + i*2;
        int childPos = keyPos + 1;
        if (keyPos < NODE_SIZE && childPos < NODE_SIZE) {
            int c = parent[childPos];
            if (c == childIdx) return i;
        }
    }
    return -1;
}

pair<int,int> getThePostionOfRecordWithNode(char* filename, int RecordID) {
    fstream f(filename, ios::in | ios::binary);
    if (!f.is_open()) {
        cout << "file Can't be open\n";
        return {-1,-1};
    }
    int nodeIndex = 1;
    vector<int> node(NODE_SIZE);
    while (true) {
        ReadNode(f, nodeIndex, node);
        if (node[0] == 0) {
            for (int i = 0; i < M; ++i) {
                int keyIdx = 1 + i*2;
                if (keyIdx < NODE_SIZE && node[keyIdx] == RecordID) {
                    return {nodeIndex, i};
                }
            }
            return {-1,-1};
        }
        int next = PickChild(node, RecordID);
        if (next == -1) {
            return {-1,-1};
        }
        nodeIndex = next;
    }
}
void BorrowFromRightLeaf(fstream &file, int nodeIndex, int rightIndex, int parentIndex) {
    vector<int> node(NODE_SIZE), right(NODE_SIZE), parent(NODE_SIZE);
    ReadNode(file, nodeIndex, node);
    ReadNode(file, rightIndex, right);
    ReadNode(file, parentIndex, parent);

    // Find leftmost key in right sibling
    int rightKey = -1, rightVal = -1, rightPos = -1;
    for (int i = 0; i < M; i++) {
        int k = right[1 + i*2];
        int v = right[1 + i*2 + 1];
        if (k != -1) {
            rightKey = k;
            rightVal = v;
            rightPos = i;
            break;
        }
    }
    if (rightPos == -1) {
        return;
    } // no keys in right sibling

    // Insert key/value at the end of underflowed leaf
    int insertPos = -1;
    for (int i = M-1; i >= 0; i--) {
        if (node[1 + i*2] == -1) {
            insertPos = i;
            break;
        }
    }
    if (insertPos == -1) {
        return; 
    }

    node[1 + insertPos*2]     = rightKey;
    node[1 + insertPos*2 + 1] = rightVal;

    // Remove key/value from right sibling (shift left)
    for (int i = rightPos; i < M-1; i++) {
        right[1 + i*2]     = right[1 + (i+1)*2];
        right[1 + i*2 + 1] = right[1 + (i+1)*2 + 1];
    }
    // clear last key and pointer
    right[1 + (M-1)*2]     = -1;
    right[1 + (M-1)*2 + 1] = -1;

    // Update parent's separator key
    // Find the slot in parent pointing to nodeIndex
    int parentSlot = FindChildSlotInParent(file, parentIndex, nodeIndex);
    if (parentSlot != -1) {
        // Separator key should be the max key of this node after borrowing
        int newMax = -1;
        for (int i = 0; i < M; i++) {
            int k = node[1 + i*2];
            if (k != -1) {
                newMax = k;
            }
        }
        parent[1 + parentSlot*2] = newMax;
        WriteNode(file, parentIndex, parent);
    }

    // Write back updated nodes
    WriteNode(file, nodeIndex, node);
    WriteNode(file, rightIndex, right);

    // Propagate parent's max if necessary
    RefreshParentMaxKeyForLeafAbdo(file, nodeIndex);
    RefreshParentMaxKeyForLeafAbdo(file, rightIndex);
}

void BorrowFromRightInternal(fstream &file, int nodeIndex, int rightIndex, int parentIndex, int sepIndex) {
    vector<int> node(NODE_SIZE), right(NODE_SIZE), parent(NODE_SIZE);
    ReadNode(file, nodeIndex, node);
    ReadNode(file, rightIndex, right);
    ReadNode(file, parentIndex, parent);

    // Get parent separator key
    int parentKey = parent[1 + sepIndex*2];

    // Insert parent key into underflowed node at rightmost slot
    int insertPos = -1;
    for (int i = M-1; i >= 0; i--) {
        if (node[1 + i*2] == -1) {
            insertPos = i;
            break;
        }
    }
    if (insertPos == -1) {
        return;
    }

    // Shift keys if necessary
    node[1 + insertPos*2]     = parentKey;
    node[1 + insertPos*2 + 1] = right[1]; // adopt leftmost child pointer of right sibling

    // Move leftmost key from right sibling up into parent
    int rightFirstKey = right[1];
    parent[1 + sepIndex*2] = rightFirstKey;

    // Remove first key and first child from right sibling (shift left)
    for (int i = 1; i < M-1; i++) {
        right[1 + (i-1)*2]     = right[1 + i*2];
        right[1 + (i-1)*2 + 1] = right[1 + i*2 + 1];
    }
    // clear last key and pointer
    right[1 + (M-1)*2]     = -1;
    right[1 + (M-1)*2 + 1] = -1;

    // Write updated nodes back
    WriteNode(file, nodeIndex, node);
    WriteNode(file, rightIndex, right);
    WriteNode(file, parentIndex, parent);

    // Propagate max keys if needed
    RefreshParentMaxKeyForLeafAbdo(file, nodeIndex);
    RefreshParentMaxKeyForLeafAbdo(file, rightIndex);
}
void BorrowFromLeftLeaf(fstream &file, int nodeIndex, int leftIndex, int parentIndex) {
    vector<int> node(NODE_SIZE), left(NODE_SIZE), parent(NODE_SIZE);
    ReadNode(file, nodeIndex, node);
    ReadNode(file, leftIndex, left);
    ReadNode(file, parentIndex, parent);

    // Find rightmost key in left sibling
    int leftKey = -1, leftVal = -1, leftPos = -1;
    for (int i = M-1; i >=0; i--) {
        int k = left[1 + i*2];
        int v = left[1 + i*2 +1];
        if (k != -1) {
            leftKey = k;
            leftVal = v;
            leftPos = i;
            break;
        }
    }
    if (leftPos == -1) {
        return;
    } // no keys in left sibling

    // Shift node keys right to make space at index 0
    for (int i = M-1; i > 0; i--) {
        node[1 + i*2]     = node[1 + (i-1)*2];
        node[1 + i*2 + 1] = node[1 + (i-1)*2 +1];
    }
    node[1] = leftKey;
    node[2] = leftVal;

    // Remove key/value from left sibling
    left[1 + leftPos*2]     = -1;
    left[1 + leftPos*2 + 1] = -1;

    // Update parent separator
    int parentSlot = FindChildSlotInParent(file, parentIndex, nodeIndex);
    if (parentSlot != -1) {
        int newMax = -1;
        for (int i = 0; i < M; i++) {
            int k = node[1 + i*2];
            if (k != -1) {
                newMax = k;
            }
        }
        parent[1 + parentSlot*2] = newMax;
        WriteNode(file, parentIndex, parent);
    }

    // Write back updated nodes
    WriteNode(file, nodeIndex, node);
    WriteNode(file, leftIndex, left);

    // Refresh keys upwards
    RefreshParentMaxKeyForLeafAbdo(file, nodeIndex);
    RefreshParentMaxKeyForLeafAbdo(file, leftIndex);
}
void BorrowFromLeftInternal(fstream &file, int nodeIndex, int leftIndex, int parentIndex, int sepIndex) {
    vector<int> node(NODE_SIZE), left(NODE_SIZE), parent(NODE_SIZE);
    ReadNode(file, nodeIndex, node);
    ReadNode(file, leftIndex, left);
    ReadNode(file, parentIndex, parent);

    // 1Get parent separator key
    int parentKey = parent[1 + sepIndex*2];

    //  Shift node keys and child pointers right to make space at beginning
    for (int i = M-1; i > 0; i--) {
        node[1 + i*2]     = node[1 + (i-1)*2];
        node[1 + i*2 + 1] = node[1 + (i-1)*2 + 1];
    }

    // Take rightmost key + last child pointer from left sibling
    int leftCount = 0;
    for (int i = 0; i < M; i++) {
        if (left[1 + i*2] != -1) {
            leftCount++;
        }
    }

    int leftKey = -1, leftChild = -1;
    for (int i = M-1; i >=0; i--) {
        if (left[1 + i*2] != -1) {
            leftKey = left[1 + i*2];
            leftChild = left[1 + i*2 +1];
            left[1 + i*2] = -1;
            left[1 + i*2 +1] = -1;
            break;
        }
    }

    // Insert parent key at start of node, leftChild as its child
    node[1] = parentKey;
    node[2] = leftChild;

    // Move left sibling key up into parent
    parent[1 + sepIndex*2] = leftKey;

    // Write back nodes
    WriteNode(file, nodeIndex, node);
    WriteNode(file, leftIndex, left);
    WriteNode(file, parentIndex, parent);

    // Refresh keys upwards
    RefreshParentMaxKeyForLeafAbdo(file, nodeIndex);
    RefreshParentMaxKeyForLeafAbdo(file, leftIndex);
}
void DeleteRecordFromIndex(char* filename, int RecordID) {

    fstream f(filename, ios::in | ios::out | ios::binary);
    if (!f.is_open()) {
        cout << "file Can't be open " << endl;
        return;
    }

    // Locate the key
    pair<int,int> NodeNum = getThePostionOfRecordWithNode(filename, RecordID);
    int nodeIndex = NodeNum.first;
    int slot      = NodeNum.second;

    if (nodeIndex == -1) {
        cout << "Not Found\n";
        return;
    }

    vector<int> node(NODE_SIZE);
    ReadNode(f, nodeIndex, node);

    // Compute old max key before deletion
    int oldLeafMax = -1;
    if (node[0] == 0) { // leaf
        for (int i = 0; i < M; ++i) {
            int k = node[1 + i*2];
            if (k != -1) {
                oldLeafMax = k;
            }
        }
    }

    // Count keys and minKeys
    int cnt = 0;
    for (int i = 0; i < M; ++i) {
        if (node[1 + i*2] != -1){ 
            ++cnt;
        }
    }
    int minKeys = M / 2;

    // Safe delete without underflow
    if (node[0] == 0 && cnt - 1 >= minKeys) {
        RemovePairAtPosition(f, nodeIndex, slot);
        RefreshParentMaxKeyForLeafAbdo(f, nodeIndex, oldLeafMax);
        return;
    }

    // Delete key
    RemovePairAtPosition(f, nodeIndex, slot);

    int parentIndex = FindParentIndex(f, nodeIndex);
    if (parentIndex == -1) {
        // Root underflow (special case)
        RefreshParentMaxKeyForLeafAbdo(f, nodeIndex, oldLeafMax);
        return;
    }

    vector<int> parent(NODE_SIZE);
    ReadNode(f, parentIndex, parent);
    // Identify child in any slot to can Identify siblings
    int childSlot = FindChildSlotInParent(f, parentIndex, nodeIndex);

    // Identify siblings
    int leftSiblingIndex  = (childSlot > 0) ? parent[1 + (childSlot-1)*2 + 1] : -1;
    int rightSiblingIndex = (childSlot < M-1) ? parent[1 + (childSlot+1)*2 + 1] : -1;

    bool borrowed = false;

    // Borrow from left
    if (!borrowed && leftSiblingIndex != -1) {
        cout << "Borrow from Left" << endl;
        vector<int> left(NODE_SIZE);
        ReadNode(f, leftSiblingIndex, left);
        int leftCount = 0;
        for (int i = 0; i < M; i++){
            if (left[1 + i*2] != -1) {
                leftCount++;
            }
        }
        if (leftCount > minKeys) {
            if (node[0] == 0) {
                BorrowFromLeftLeaf(f, nodeIndex, leftSiblingIndex, parentIndex);
            }
            else{ 
                BorrowFromLeftInternal(f, nodeIndex, leftSiblingIndex, parentIndex, childSlot-1);
            }
            borrowed = true;
        }
    }

    // Borrow from right
    if (!borrowed && rightSiblingIndex != -1) {
        cout << "Borrow from Right" << endl;
        vector<int> right(NODE_SIZE);
        ReadNode(f, rightSiblingIndex, right);
        int rightCount = 0;
        for (int i = 0; i < M; i++) {
            if (right[1 + i*2] != -1) {
                rightCount++;
            }
        }
        if (rightCount > minKeys) {
            if (node[0] == 0) {
                BorrowFromRightLeaf(f, nodeIndex, rightSiblingIndex, parentIndex);
            }
            else {
                BorrowFromRightInternal(f, nodeIndex, rightSiblingIndex, parentIndex, childSlot);
            }
            borrowed = true;
        }
    }

    // Merge if borrowing failed
    // if (!borrowed) {
    //     if (leftSiblingIndex != -1) {
    //         if (node[0] == 0) MergeLeaves(f, leftSiblingIndex, nodeIndex, parentIndex, childSlot-1);
    //         else MergeInternals(f, leftSiblingIndex, nodeIndex, parentIndex, childSlot-1);
    //     } else if (rightSiblingIndex != -1) {
    //         if (node[0] == 0) MergeLeaves(f, nodeIndex, rightSiblingIndex, parentIndex, childSlot);
    //         else MergeInternals(f, nodeIndex, rightSiblingIndex, parentIndex, childSlot);
    //     } else {
    //         cout << "Underflow: single child root?" << endl;
    //     }
    // }
    if(!borrowed){
        cout << "Merge";
        return;
    }

    // Refresh parent keys
    RefreshParentMaxKeyForLeafAbdo(f, nodeIndex, oldLeafMax);
}

int main() {
    char filename[] = "indexfile.bin";

    CreateIndexFileFile(filename, 10, M);
    DisplayIndexFileContent(filename);
    
    // FIRST BATCH
    InsertNewRecordAtIndex(filename, 3, 12);
    InsertNewRecordAtIndex(filename, 7, 24);
    InsertNewRecordAtIndex(filename, 10, 48);
    InsertNewRecordAtIndex(filename, 24, 60);
    InsertNewRecordAtIndex(filename, 14, 72);
    
    cout << "\nAfter first batch:\n";
    DisplayIndexFileContent(filename);
    
    // TRIGGER FIRST SPLIT
    InsertNewRecordAtIndex(filename, 19, 84);
    cout << "\nAfter inserting 19:\n";
    DisplayIndexFileContent(filename);
    
    InsertNewRecordAtIndex(filename, 30, 196);
    InsertNewRecordAtIndex(filename, 15, 108);
    InsertNewRecordAtIndex(filename, 1, 120);
    InsertNewRecordAtIndex(filename, 5, 132);
    cout << "\nAfter second batch:\n";
    DisplayIndexFileContent(filename);
    
    // Trigger second split
    InsertNewRecordAtIndex(filename, 2, 144);
    cout << "\nAfter inserting 2:\n";
    DisplayIndexFileContent(filename);
    //third patch
    InsertNewRecordAtIndex(filename, 8, 156);
    InsertNewRecordAtIndex(filename, 9, 168);
    InsertNewRecordAtIndex(filename, 6, 180);
    InsertNewRecordAtIndex(filename, 11, 192);
    InsertNewRecordAtIndex(filename, 12, 204);
    InsertNewRecordAtIndex(filename, 17, 216);
    InsertNewRecordAtIndex(filename, 18, 228);
    InsertNewRecordAtIndex(filename, 32, 240);

    DisplayIndexFileContent(filename);

    DeleteRecordFromIndex(filename, 10);

    DisplayIndexFileContent(filename);
    DeleteRecordFromIndex(filename, 9);
    DisplayIndexFileContent(filename);
    
    cout << SearchARecord(filename,10);

    return 0;
}


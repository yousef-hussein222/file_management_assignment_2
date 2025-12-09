#include <bits/stdc++.h>
using namespace std;


const int fileNameSize = 20;
const int numberOfRecords = 10;
const int descendant = 5;
const int numberOfColumns = descendant * 2 +1;

void writeToFile(fstream& file,int nodeIndex,const vector<int>&row) {

    file.seekp(nodeIndex * numberOfColumns * sizeof(int),ios::beg);
    file.write((char*)row.data(),numberOfColumns * sizeof(int));
}

void readFromFile(fstream& file,int nodeIndex, vector<int>&row) {
    file.seekg(nodeIndex * numberOfColumns * sizeof(int),ios::beg);
    file.read((char*)row.data(),numberOfColumns * sizeof(int));
}

int numberOfKeys(const vector<int>& row) {
    int keysNumber = 0;
    for(int i = 1;i<row.size();i+=2) {
        if(row[i] != -1) {
            keysNumber++;
        }
    }
    return keysNumber;
}
//=============================   helper Function (delete) for abdo yasser =============================================
int minimumKeys(){
    return ceil(descendant / 2.0);
}


int nodeKeyCount(const vector<int> &row){
    return numberOfKeys(row);
}

vector<pair<int,int>> getPairs(const vector<int> &node) {
    vector<pair<int,int>> pairs;
    for (int i = 1; i < numberOfColumns; i += 2) {
        if (node[i] != -1) {
            int key = node[i];
            int ptr = -1;
            if (i + 1 < numberOfColumns) {
                ptr = node[i+1];
            }
            pairs.emplace_back(key, ptr);
        }
    }
    return pairs;
}

void putPairs(vector<int> &node, const vector<pair<int,int>>& pairs, int type) {
    // initialize node
    node.assign(numberOfColumns, -1);
    node[0] = type;
    for (size_t i = 0; i < pairs.size(); ++i) {
        int keyIdx = 1 + 2 * i;
        if (keyIdx < numberOfColumns) {
            node[keyIdx] = pairs[i].first;
            if (keyIdx + 1 < numberOfColumns) {
                node[keyIdx+1] = pairs[i].second;
            }
        }
    }
}

vector<int> getChildren(const vector<int> & node){
    vector<int> children;
    for (int i = 1; i < numberOfColumns; i += 2) {
        int c = node[i];
        if (c != -1) {
            children.push_back(c);
        }
    }
    return children;
}

int findParentHelper(fstream& file, int current, int target){
    /*
        current = ana abo7a;
        terget = ele 3aizien ne3raf men aboo
    */
    // we start form root ya ismail, so if current = target  then  no  parent
    if(current == target){
        return -1;
    }
    vector<int> node(numberOfColumns);
    readFromFile(file, current, node);
    // when = 0 hence this empty node ya ismail
    if(node.size() == 0 || node[0] == 0 || node[0] == -1){
        return -1;
    }
    vector<int> children = getChildren(node);
    for(int child : children){
        if(child == target){
            return current;
        }
        int result = findParentHelper(file, child, target);
        if (result != -1){
            return result;
        }
    }
    return -1;
}


int findParent(fstream& file, int nodeIndex) {
    if (nodeIndex == 1) {
        return -1;
    }
    return findParentHelper(file, 1, nodeIndex);
}

pair<int, int> findNearestSiblings(fstream& file, int nodeIndex){
    // root mesh 3ando siblings
    if(nodeIndex == 1){
        return make_pair(-1,-1);
    }

    int parent = findParent(file, nodeIndex);

    if(parent == 1){
        return make_pair(-1,-1);
    }
    vector<int> parentNode(numberOfColumns);
    readFromFile(file, parent, parentNode);

    vector<int> children = getChildren(parentNode);
    int position = -1;

    for(int i = 0; i < (int) children.size(); i++ ){
        if(children[i] == nodeIndex){
            position = i;
            break;
        }
    }

    if(position == -1){
        return make_pair(-1,-1);
    }
    int left = -1, right = -1;
    if (position > 0){
        left = children[position-1];
    }
    if(position < (int) children.size() - 1){
        right = children[position + 1];
    }
    return make_pair(left, right);

}

void deallocateNode(fstream& file, int nodeIndex){
    vector<int> header(numberOfColumns);
    readFromFile(file, 0, header);

    int next = -1;
    if (header.size() > 1) {
        next = header[1];
    }
    header[1] = nodeIndex;
    writeToFile(file, 0, header);

    vector<int> node(numberOfColumns, -1);
    // mark node as free/empty type (0)
    node[0] = 0;
    // store next pointer in first child slot so free list points correctly
    if (numberOfColumns > 1) {
        node[1] = next;
    }
    writeToFile(file, nodeIndex, node);
}

pair<int,int> findKeyNode(fstream& file, int recordID) {
    int nodeIndex = 1;
    vector<int> node(numberOfColumns);
    readFromFile(file, nodeIndex, node);

    // if tree empty
    if (node.size() == 0 || node[0] == -1 || node[0] == 0) {
        return make_pair(-1, -1);
    }

    // traverse down until leaf
    while (node[0] == 1) { // internal
        bool found = false;

        // children are at odd indices; keys at even indices (starting at 2)
        // walk keys from left to right to decide which child to descend to
        int childToDescend = -1;

        // special case: if first key exists and recordID < first key, go to child0
        if (numberOfColumns > 2 && node[2] != -1 && recordID < node[2]) {
            childToDescend = node[1]; // child0
            found = true;
        } 
        else {
            // general case: scan keys left-to-right
            for (int keyIndex = 4; keyIndex < numberOfColumns; keyIndex += 2) {
                if (node[keyIndex] != -1 && recordID < node[keyIndex]) {
                    // child sits at keyIdx-1
                    childToDescend = node[keyIndex - 1];
                    found = true;
                    break;
                }
            }
        }

        // If not found and record is larger than all keys, go to last child
        if (!found) {
            // last child index is the highest odd index < numberOfColumns
            int lastChildIndex = numberOfColumns - 1;
            if (lastChildIndex % 2 == 0) lastChildIndex--; // ensure odd
            childToDescend = node[lastChildIndex];
        }

        if (childToDescend == -1) {
            // not found or malformed node
            return make_pair(-1, -1);
        }
        nodeIndex = childToDescend;
        readFromFile(file, nodeIndex, node);
    }

    // now node is leaf - search keys
    for (int i = 1, keyNum = 0; i < numberOfColumns; i += 2, ++keyNum) {
        if (node[i] == recordID) {
            return make_pair(nodeIndex, keyNum);
        }
    }
    return make_pair(-1,-1);
}

void removeAtPosition(fstream& file, int nodeIndex, int position) {
    vector<int> node(numberOfColumns);
    readFromFile(file, nodeIndex, node);

    bool isLeaf = (node[0] == 0);

    if (isLeaf) {
        // shift keys left starting from 'position'
        for (int i = position; i < descendant - 1; ++i) {
            int fromKeyIndex = 1 + 2 * (i + 1);
            int fromPtrIndex = 2 + 2 * (i + 1);
            int toKeyIndex = 1 + 2 * i;
            int toPtrIndex = 2 + 2 * i;

            if (fromKeyIndex < numberOfColumns) {
                node[toKeyIndex] = node[fromKeyIndex];
            }
            else {
                node[toKeyIndex] = -1;
            }
            if (fromPtrIndex < numberOfColumns) {
                node[toPtrIndex] = node[fromPtrIndex];
            }
            else {
                node[toPtrIndex] = -1;
            }
        }
        // clear last slot
        int lastKeyIndex = 1 + 2 * (descendant - 1);
        int lastPtrIndex = 2 + 2 * (descendant - 1);
        if (lastKeyIndex < numberOfColumns) {
            node[lastKeyIndex] = -1;
        }
        if (lastPtrIndex < numberOfColumns) {
            node[lastPtrIndex] = -1;
        }
    } 
    else {
        // Internal node: remove the key at 'position' (position corresponds to key slot)
        // We need to shift subsequent keys and corresponding children left
        // children layout: c0, k0, c1, k1, c2, k2, ...; removing key at pos means removing key at index 2 + 2*pos
        int removeKeyIndex = 2 + 2 * position;
        int removeChildIndex = 1 + 2 * (position + 1); // the right child that becomes merged/shifting

        // shift keys left
        for (int i = position; i < descendant - 1; ++i) {
            int srcKeyIndex = 2 + 2 * (i + 1);
            int dstKeyIndex = 2 + 2 * i;
            node[dstKeyIndex] = (srcKeyIndex < numberOfColumns) ? node[srcKeyIndex] : -1;
        }
        // clear last key
        int lastKeyIndex = 2 + 2 * (descendant - 1);
        if (lastKeyIndex < numberOfColumns) {
            node[lastKeyIndex] = -1;
        }

        // shift children left starting from removeChildIdx
        for (int i = position + 1; i < descendant; ++i) {
            int srcChildIndex = 1 + 2 * (i + 1);
            int dstChildIndex = 1 + 2 * i;
            if (srcChildIndex < numberOfColumns) {
                node[dstChildIndex] = node[srcChildIndex];
            }
            else node[dstChildIndex] = -1;
        }
        // clear last child slot
        int lastChildIdx = 1 + 2 * descendant;
        // last valid child index is numberOfColumns - 1 or numberOfColumns - 2 depending on layout, clear the highest odd index
        int highestOdd = numberOfColumns - 1;
        if (highestOdd % 2 == 0) {
            highestOdd--;
        }
        if (highestOdd >= 1 && highestOdd < numberOfColumns) {
            node[highestOdd] = -1;
        }
    }

    writeToFile(file, nodeIndex, node);
}

bool borrowFromRight(fstream& file, int nodeIndex, int rightIndex, int parentIndex) {
    vector<int> node(numberOfColumns);
    vector<int> right(numberOfColumns);
    vector<int> parent(numberOfColumns);
    readFromFile(file, nodeIndex, node);
    readFromFile(file, rightIndex, right);
    readFromFile(file, parentIndex, parent);

    int nodeKeys = nodeKeyCount(node);
    int rightKeys = nodeKeyCount(right);
    int minKeys = minimumKeys();

    // Cannot borrow if right has only minimum keys
    if (rightKeys <= minKeys){ 
        return false;
    }

    bool isLeaf = (node[0] == 0);

    if (isLeaf) {
        // 1. Borrow first key from right leaf
        int borrowedKeyIndex = -1;
        for (int i = 1; i < numberOfColumns; i += 2) {
            if (right[i] != -1) { 
                borrowedKeyIndex = i; 
                break; 
            }
        }
        if (borrowedKeyIndex == -1) {
            return false;
        }

        int borrowedKey = right[borrowedKeyIndex];
        int borrowedPtr = (borrowedKeyIndex + 1 < numberOfColumns) ? right[borrowedKeyIndex + 1] : -1;

        // 2. Remove borrowed key from right (shift left)
        for (int i = borrowedKeyIndex; i < numberOfColumns - 2; i += 2) {
            right[i] = right[i + 2];
            right[i + 1] = right[i + 3];
        }
        right[numberOfColumns - 2] = right[numberOfColumns - 1] = -1;
        writeToFile(file, rightIndex, right);

        // 3. Insert borrowed key into node in sorted order
        int insertPosition = 0;
        while (insertPosition < descendant && node[1 + 2 * insertPosition] != -1 && node[1 + 2 * insertPosition] < borrowedKey){
            insertPosition++;
        }

        // Shift node to make space
        for (int i = descendant - 1; i > insertPosition; --i) {
            node[1 + 2 * i] = node[1 + 2 * (i - 1)];
            node[2 + 2 * i] = node[2 + 2 * (i - 1)];
        }
        node[1 + 2 * insertPosition] = borrowedKey;
        node[2 + 2 * insertPosition] = borrowedPtr;
        writeToFile(file, nodeIndex, node);

        // 4. Update parent separator key
        vector<int> parentChildren = getChildren(parent);
        int parentPosition = -1;
        for (int i = 0; i < parentChildren.size(); i++) {
            if (parentChildren[i] == nodeIndex) { 
                parentPosition = i; 
                break; 
            }
        }
            
        if (parentPosition >= 0 && parentPosition < parentChildren.size() - 1) {
            // separator index in parent
            int sepIndex = 1 + 2 * parentPosition;
            // new separator = first key in right
            int newSep = -1;
            for (int i = 1; i < numberOfColumns; i += 2) { 
                if (right[i] != -1) {
                    newSep = right[i];
                    break; 
                } 
            }
            if (sepIndex < numberOfColumns) {
                parent[sepIndex] = newSep;
            }
            writeToFile(file, parentIndex, parent);
        }

        return true;
    } 
    else {
        // Internal node: borrow first child and key from right
        int borrowedChild = -1, borrowedKey = -1;
        for (int i = 1; i < numberOfColumns; i += 2) { 
            if (right[i] != -1) { 
                borrowedChild = right[i]; 
                break; 
            } 
        }
        for (int i = 2; i < numberOfColumns; i += 2) { 
            if (right[i] != -1) { 
                borrowedKey = right[i]; 
                break; 
            } 
        }
        if (borrowedKey == -1) {
            return false;
        }

        // Remove first child/key from right (shift left)
        for (int i = 0; i < descendant - 1; ++i) {
            int srcChild = 1 + 2 * (i + 1), dstChild = 1 + 2 * i;
            int srcKey = 2 + 2 * (i + 1), dstKey = 2 + 2 * i;
            right[dstChild] = (srcChild < numberOfColumns) ? right[srcChild] : -1;
            right[dstKey] = (srcKey < numberOfColumns) ? right[srcKey] : -1;
        }
        right[1 + 2 * (descendant - 1)] = right[2 + 2 * (descendant - 1)] = -1;
        writeToFile(file, rightIndex, right);

        // Move parent separator key into node
        vector<int> parentChildren = getChildren(parent);
        int parentPosition = -1;
        for (int i = 0; i < parentChildren.size(); i++) {
            if (parentChildren[i] == nodeIndex) { 
                parentPosition = i; 
                break; 
            }
        }
        
        if (parentPosition == -1) {
            return false;
        }

        int parentSepIdx = 1 + 2 * parentPosition;
        int parentKey = (parentSepIdx < numberOfColumns) ? parent[parentSepIdx] : -1;

        // Rebuild node: oldChildren + borrowedChild, oldKeys + parentKey
        vector<int> oldChildren = getChildren(node);
        vector<int> oldKeys;
        for (int i = 2; i < numberOfColumns; i += 2) {
            if (node[i] != -1){ 
                oldKeys.push_back(node[i]);
            }
        }

        vector<int> newNode(numberOfColumns, -1);
        newNode[0] = 1;
        int cIdx = 1;
        for (int c : oldChildren) { 
            newNode[cIdx] = c; 
            cIdx += 2; 
        }
        newNode[cIdx] = borrowedChild;

        int kIdx = 2;
        for (int k : oldKeys) { 
            newNode[kIdx] = k; 
            kIdx += 2; 
        }
        newNode[kIdx] = parentKey;

        writeToFile(file, nodeIndex, newNode);

        // Update parent separator to borrowedKey
        if (parentSepIdx < numberOfColumns) {
            parent[parentSepIdx] = borrowedKey;
        }
        writeToFile(file, parentIndex, parent);

        return true;
    }
}


// Borrow from left sibling (mirror of borrowFromRight)
bool borrowFromLeft(fstream& file, int nodeIndex, int leftIndex, int parentIndex) {
    vector<int> node(numberOfColumns);
    vector<int> left(numberOfColumns);
    vector<int> parent(numberOfColumns);
    readFromFile(file, nodeIndex, node);
    readFromFile(file, leftIndex, left);
    readFromFile(file, parentIndex, parent);

    int nodeKeys = nodeKeyCount(node);
    int leftKeys = nodeKeyCount(left);
    int minKeys = minimumKeys();

    if (leftKeys <= minKeys) return false;

    bool isLeaf = (node[0] == 0);

    if (isLeaf) {
        // Borrow last key from left
        int borrowedKey = left[1 + 2 * (descendant - 1)];
        int borrowedPtr = left[2 + 2 * (descendant - 1)];

        // Clear last of left and shift rightwards
        for (int i = descendant - 1; i > 0; i--) {
            left[1 + 2 * i] = left[1 + 2 * (i - 1)];
            left[2 + 2 * i] = left[2 + 2 * (i - 1)];
        }
        left[1] = -1; left[2] = -1;
        writeToFile(file, leftIndex, left);

        // insert borrowed at beginning of node
        for (int i = descendant - 1; i > 0; i--) {
            node[1 + 2 * i] = node[1 + 2 * (i - 1)];
            node[2 + 2 * i] = node[2 + 2 * (i - 1)];
        }
        node[1] = borrowedKey;
        node[2] = borrowedPtr;
        writeToFile(file, nodeIndex, node);

        // Update parent separator key (parent key between left and node)
        vector<int> children = getChildren(parent);
        int parentPosition = -1;
        for (int i = 0; i < children.size(); i++){
            if (children[i] == nodeIndex) { parentPosition = i; break; }
        }
        if (parentPosition > 0) {
            parent[1 + 2 * (parentPosition - 1)] = left[1 + 2 * (descendant - 1)];
            writeToFile(file, parentIndex, parent);
        }

        return true;
    } else {
        // Internal node: borrow last key and child from left
        int borrowedKey = left[2 + 2 * (descendant - 1)];
        int borrowedChild = left[1 + 2 * (descendant - 1)];

        // remove last key/child from left
        left[2 + 2 * (descendant - 1)] = -1;
        left[1 + 2 * (descendant - 1)] = -1;
        writeToFile(file, leftIndex, left);

        // get parent's separator key between left and node
        vector<int> parentChildren = getChildren(parent);
        int parentPosition = -1;
        for (int i = 0; i < parentChildren.size(); i++){
            if (parentChildren[i] == nodeIndex) { parentPosition = i; break; }
        }
        int parentKey = parent[1 + 2 * (parentPosition - 1)];

        // new node will have parentKey appended at its beginning of keys
        vector<int> oldChildren = getChildren(node);
        vector<int> oldKeys;
        for (int k = 2; k < numberOfColumns; k += 2) if (node[k] != -1) oldKeys.push_back(node[k]);

        // rebuild node
        vector<int> newNode(numberOfColumns, -1);
        newNode[0] = 1;
        // set children: borrowedChild then old children
        int childIdx = 1;
        newNode[childIdx] = borrowedChild; childIdx += 2;
        for (int c : oldChildren) { newNode[childIdx] = c; childIdx += 2; }

        // set keys: parentKey then oldKeys
        int keyIdx = 2;
        newNode[keyIdx] = parentKey; keyIdx += 2;
        for (int k : oldKeys) { newNode[keyIdx] = k; keyIdx += 2; }

        writeToFile(file, nodeIndex, newNode);

        // update parent separator to borrowedKey
        parent[1 + 2 * (parentPosition - 1)] = borrowedKey;
        writeToFile(file, parentIndex, parent);

        return true;
    }
}

// Merge right into left and remove separator from parent
void mergeWithSiblingAndDeleteFromParent(fstream& file, int leftIndex, int rightIndex, int parentIndex) {
    vector<int> left(numberOfColumns), right(numberOfColumns), parent(numberOfColumns);
    readFromFile(file, leftIndex, left);
    readFromFile(file, rightIndex, right);
    readFromFile(file, parentIndex, parent);

    // find positions
    vector<int> parentChildren = getChildren(parent);
    int leftPos = -1;
    for (int i = 0; i < parentChildren.size(); ++i) if (parentChildren[i] == leftIndex) leftPos = i;
    if (leftPos == -1) return; // shouldn't happen

    int sepPos = leftPos; // separator key index in parent is at 1 + 2*sepPos
    int separatorKey = parent[1 + 2 * sepPos];

    bool leftIsLeaf = (left[0] == 0);

    if (leftIsLeaf) {
        // append all keys from right to left
        // find last occupied pos in left
        int lcount = nodeKeyCount(left);
        int rcount = nodeKeyCount(right);
        for (int i = 0; i < rcount; ++i) {
            left[1 + 2 * (lcount + i)] = right[1 + 2 * i];
            left[2 + 2 * (lcount + i)] = right[2 + 2 * i];
        }
        // clear right
        vector<int> empty(numberOfColumns, -1);
        writeToFile(file, leftIndex, left);
        deallocateNode(file, rightIndex);

    } else {
        // internal: append separator key then right's keys and children
        vector<int> leftChildren = getChildren(left);
        vector<int> rightChildren = getChildren(right);

        vector<int> leftKeys;
        for (int k = 2; k < numberOfColumns; k += 2) if (left[k] != -1) leftKeys.push_back(left[k]);
        vector<int> rightKeys;
        for (int k = 2; k < numberOfColumns; k += 2) if (right[k] != -1) rightKeys.push_back(right[k]);

        // rebuild left: children = leftChildren + rightChildren
        vector<int> newLeft(numberOfColumns, -1);
        newLeft[0] = 1;
        int childIdx = 1;
        for (int c : leftChildren) { newLeft[childIdx] = c; childIdx += 2; }
        for (int c : rightChildren) { newLeft[childIdx] = c; childIdx += 2; }

        // keys = leftKeys + separatorKey + rightKeys
        int keyIdx = 2;
        for (int k : leftKeys) { newLeft[keyIdx] = k; keyIdx += 2; }
        newLeft[keyIdx] = separatorKey; keyIdx += 2;
        for (int k : rightKeys) { newLeft[keyIdx] = k; keyIdx += 2; }

        writeToFile(file, leftIndex, newLeft);
        deallocateNode(file, rightIndex);
    }

    // remove separator key from parent at position sepPos
    removeAtPosition(file, parentIndex, sepPos);

    // if parent becomes empty root case not handled fully (would require root collapse)
    // else if parent underflows, fix recursively
    vector<int> parentNow(numberOfColumns);
    readFromFile(file, parentIndex, parentNow);
    if (parentIndex != 1 && nodeKeyCount(parentNow) < minimumKeys()) {
        // try to fix parent underflow
        // find siblings of parent and attempt borrowing/merging
        int gp = findParent(file, parentIndex);
        if (gp != -1) {
            auto sibs = findNearestSiblings(file, parentIndex);
            bool fixed = false;
            if (sibs.first != -1) {
                fixed = borrowFromLeft(file, parentIndex, sibs.first, gp);
            }
            if (!fixed && sibs.second != -1) {
                fixed = borrowFromRight(file, parentIndex, sibs.second, gp);
            }
            if (!fixed) {
                if (sibs.first != -1) {
                    mergeWithSiblingAndDeleteFromParent(file, sibs.first, parentIndex, gp);
                } else if (sibs.second != -1) {
                    mergeWithSiblingAndDeleteFromParent(file, parentIndex, sibs.second, gp);
                }
            }
        }
    }
}


// ========================================== delete function =================================================
void  DeleteRecordFromIndexHelper(fstream& file, int recordID, int nodeIndex) {
    auto[foundNode, Position] = findKeyNode(file, recordID);
    if(foundNode == -1) {
        cout << "Node Not Found!!" << endl;
        return;
    }

    vector<int> node(numberOfColumns);
    readFromFile(file, foundNode, node);
    int nodeCountKeys = nodeKeyCount(node);
    int minimumkeys = minimumKeys();
    // to show if this key is greatest in node or not
    bool isLargest = (Position == nodeCountKeys - 1);
    // Rule 1 & 2: if more than min, delete and update if largest
    if(nodeCountKeys > minimumkeys ){
        removeAtPosition(file, foundNode, Position);
        // If deleted key was largest, update parent's separator key
        if (isLargest && foundNode != 1) { // not root
            int parent = findParent(file, foundNode);
            if (parent != -1) {
                vector<int> parentNode(numberOfColumns);
                readFromFile(file, parent, parentNode);
                vector<int> children = getChildren(parentNode);
                int parentPosition = -1;
                for (int i = 0; i < children.size(); i++) {
                    if (children[i] == foundNode) {
                        parentPosition = i;
                        break;
                    }
                }

                if (parentPosition > 0) { // has left sibling
                    // Re-read the node to find new largest
                    readFromFile(file, foundNode, node);
                    int newLargest = -1;
                    for (int i = numberOfColumns - 2; i >= 1; i -= 2) {
                        if (node[i] != -1) {
                            newLargest = node[i];
                            break;
                        }
                    }
                    // Update parent's separator key
                    parentNode[1 + 2 * (parentPosition - 1)] = newLargest;
                    writeToFile(file, parent, parentNode);
                }
            }
        }
        return;
    }

    // Rule 3 & 4: exactly min keys
    removeAtPosition(file, foundNode, Position);
    if (foundNode == 1) {
        return;
    } // root
    int parent = findParent(file, foundNode);
    auto sibs = findNearestSiblings(file, foundNode);
    bool fixed = false;
    // try borrow from left
    if (sibs.first != -1) {
        fixed = borrowFromLeft(file, foundNode, sibs.first, parent);
    }
    // try borrow from right if not fixed
    if (!fixed && sibs.second != -1) {
        fixed = borrowFromRight(file, foundNode, sibs.second, parent);
    }
    if (!fixed) {
        // merge
        if (sibs.first != -1) {
            mergeWithSiblingAndDeleteFromParent(file, sibs.first, foundNode, parent);
        } 
        else if (sibs.second != -1) {
            mergeWithSiblingAndDeleteFromParent(file, foundNode, sibs.second, parent);
        }
    }
}


void DeleteRecordFromIndex(char* filename, int RecordID) {
    fstream indexFile(filename, ios::in | ios::out | ios::binary);
    if(!indexFile) {
        cout<< "Error happen when open index file...!" << endl;
        return;
    }
    DeleteRecordFromIndexHelper(indexFile, RecordID, 1);
    indexFile.close();
}
//====================================================================

void createIndexFile(char* filename,int numOfRecords,int m) {
    fstream indexFile(filename,ios::out | ios::binary | ios::trunc);
    if(!indexFile) {
        cout<< "Error happen when open index file...!" << endl;
        return;
    }
    int fillValue = -1;
    for(int i = 0;i<numOfRecords;i++) {
        int nextEmptySlot = (i+1 == numOfRecords) ? -1 :i+1;
        for(int j = 0;j<(m * 2) + 1;j++) {
            if(j == 1) {
                indexFile.write((char*)&nextEmptySlot,sizeof (nextEmptySlot));
            }
            else {
                indexFile.write((char *) &fillValue, sizeof(fillValue));
            }
        }
    }
    indexFile.close();
}

void displayIndexFileContent(char* fileName) {
    fstream indexFile(fileName,ios::in | ios::binary);
    if(!indexFile) {
        cout << "Error happen when open index file...!" << endl;
        return;
    }

    vector<int>nodes(numberOfColumns);


    for(int i = 0;i<numberOfRecords;i++) {
        readFromFile(indexFile,i,nodes);
        for(auto& it : nodes) {
            cout << it << " ";
        }
        cout << endl;
    }

}

int insertNewRecordAtIndex(char* filename, int RecordID, int Reference) {
    fstream indexFile(filename,ios::in | ios::out | ios::binary );
    if(!indexFile) {
        cout<< "Error happen when open index file...!" << endl;
        return -1;
    }

    vector<int>firstRowInFile(numberOfColumns),root(numberOfColumns);
    readFromFile(indexFile,0,firstRowInFile);

    int nextEmptyNode = firstRowInFile[1];

    readFromFile(indexFile,1,root);

    // root is empty
    if(root[0] == -1) {

        nextEmptyNode = root[1];
        firstRowInFile[1] = nextEmptyNode;

        writeToFile(indexFile,0,firstRowInFile);

        // mark as leaf node
        root[0] = 0;
        root[1] = RecordID;
        root[2] = Reference;

        writeToFile(indexFile,1,root);

        // index of where new (RecordID, Reference) is inserted
        return 1;
    }

    // root is the only node and in the same time is leaf
    if(root[0] == 0) {
        int counter = numberOfKeys(root);
        if(counter < descendant) {
            vector<pair<int,int>>temp;
            for(int i = 1;i<counter * 2;i+=2) {
                temp.emplace_back(root[i],root[i+1]);
            }

            // insert new (RecordID, Reference) value in the temp to sort it with the new value
            temp.emplace_back(RecordID,Reference);
            sort(temp.begin(),temp.end());

            // write back new data after to root again after sort them with the new value
            for(int i = 0;i<temp.size();i++) {
                root[1 + 2 * i] = temp[i].first;
                root[2 + 2 * i] = temp[i].second;
            }

            writeToFile(indexFile,1,root);
        }
        return 1;
    }
}

int main() {

    vector<pair<int,int>>arr = {
            {3,12},
            {7,24},
            {10,48},
            {24,60},
            {14,72},
            {19, 84},
            {30, 96} ,
            {15, 108} ,
            {1, 120 } ,
            {5, 132}  , 
            {2, 144} ,
            {8, 156} ,
            {9, 168} ,
            {6, 180} ,
            {11, 192} , 
            {12, 204} ,
            {17, 216} ,
            {18, 228} , 
    };

    char name[fileNameSize] = "BTreeIndex.bin";
    createIndexFile(name,numberOfRecords,descendant);
    displayIndexFileContent(name);
    cout << "----------------------------------" << endl;
    for(int i = 0;i<arr.size();i++){
        insertNewRecordAtIndex(name,arr[i].first,arr[i].second);
        displayIndexFileContent(name);
        cout << "----------------------------------" << endl;
    }

    return 0;
}
 

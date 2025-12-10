
#include <bits/stdc++.h>
using namespace std;


const int fileNameSize = 20;
const int numberOfRecords = 10;
const int descendant = 5;
const int numberOfColumns = descendant * 2 +1;

///joe ihab
const int M = 5;
const int NODE_SIZE = 1 + M * 2;
const int NUM_NODES = 10;

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
//=============================   helper Function (delete) for abdo yasser  =============================================
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
    if (node.size() == 0 || node[0] == -1 ) {
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

    // Cannot borrow if left has only minimum keys
    if (leftKeys <= minKeys) {
        return false;
    }

    bool isLeaf = (node[0] == 0);

    if (isLeaf) {
        // 1. Borrow last key from left leaf
        int borrowedKeyIndex = -1;
        for (int i = numberOfColumns - 2; i >= 1; i -= 2) {
            if (left[i] != -1) {
                borrowedKeyIndex = i;
                break;
            }
        }
        if (borrowedKeyIndex == -1) {
            return false;
        }

        int borrowedKey = left[borrowedKeyIndex];
        int borrowedPtr = (borrowedKeyIndex + 1 < numberOfColumns) ? left[borrowedKeyIndex + 1] : -1;

        // 2. Remove borrowed key from left (it's the last occupied, just clear it)
        left[borrowedKeyIndex] = -1;
        if (borrowedKeyIndex + 1 < numberOfColumns) left[borrowedKeyIndex + 1] = -1;
        writeToFile(file, leftIndex, left);

        // 3. Insert borrowed key into node in sorted order
        int insertPosition = 0;
        while (insertPosition < descendant && node[1 + 2 * insertPosition] != -1 && node[1 + 2 * insertPosition] < borrowedKey) {
            insertPosition++;
        }

        // Shift node to make space (shift right)
        for (int i = descendant - 1; i > insertPosition; --i) {
            node[1 + 2 * i] = node[1 + 2 * (i - 1)];
            node[2 + 2 * i] = node[2 + 2 * (i - 1)];
        }
        node[1 + 2 * insertPosition] = borrowedKey;
        node[2 + 2 * insertPosition] = borrowedPtr;
        writeToFile(file, nodeIndex, node);

        // 4. Update parent separator key (separator between left and node)
        vector<int> parentChildren = getChildren(parent);
        int parentPosition = -1;
        for (int i = 0; i < parentChildren.size(); i++) {
            if (parentChildren[i] == nodeIndex) {
                parentPosition = i;
                break;
            }
        }

        if (parentPosition > 0) {
            // separator index in parent between left and node
            int sepIndex = 1 + 2 * (parentPosition - 1);
            // new separator = last key in left (after removal)
            int newSep = -1;
            for (int i = numberOfColumns - 2; i >= 1; i -= 2) {
                if (left[i] != -1) {
                    newSep = left[i];
                    break;
                }
            }
            if (sepIndex < numberOfColumns) {
                parent[sepIndex] = newSep;
            }
            writeToFile(file, parentIndex, parent);
        }

        return true;
    } else {
        // Internal node: borrow last child and key from left
        vector<int> leftChildren = getChildren(left);
        if (leftChildren.empty()) return false;

        // get left's keys
        vector<int> leftKeysVec;
        for (int i = 2; i < numberOfColumns; i += 2) {
            if (left[i] != -1) leftKeysVec.push_back(left[i]);
        }
        if (leftKeysVec.empty()) return false;

        int borrowedChild = leftChildren.back();
        int borrowedKey = leftKeysVec.back();

        // Remove last child/key from left (clear their positions)
        int lastChildPos = 1 + 2 * (leftChildren.size() - 1);
        int lastKeyPos = 2 + 2 * (leftKeysVec.size() - 1);
        if (lastChildPos < numberOfColumns) left[lastChildPos] = -1;
        if (lastKeyPos < numberOfColumns) left[lastKeyPos] = -1;
        writeToFile(file, leftIndex, left);

        // Find parent position
        vector<int> parentChildren = getChildren(parent);
        int parentPosition = -1;
        for (int i = 0; i < parentChildren.size(); i++) {
            if (parentChildren[i] == nodeIndex) {
                parentPosition = i;
                break;
            }
        }
        if (parentPosition <= 0) {
            // no left separator to update (node is first child) -> can't borrow from left
            return false;
        }

        int parentSepIdx = 1 + 2 * (parentPosition - 1);
        int parentKey = (parentSepIdx < numberOfColumns) ? parent[parentSepIdx] : -1;

        // Rebuild node: prepend borrowedChild and parentKey before old children/keys
        vector<int> oldChildren = getChildren(node);
        vector<int> oldKeys;
        for (int i = 2; i < numberOfColumns; i += 2) {
            if (node[i] != -1) oldKeys.push_back(node[i]);
        }

        vector<int> newNode(numberOfColumns, -1);
        newNode[0] = 1;

        // children: borrowedChild then oldChildren
        int cIdx = 1;
        newNode[cIdx] = borrowedChild;
        cIdx += 2;
        for (int c : oldChildren) {
            newNode[cIdx] = c;
            cIdx += 2;
        }

        // keys: parentKey then oldKeys
        int kIdx = 2;
        if (parentKey != -1) {
            newNode[kIdx] = parentKey;
            kIdx += 2;
        }
        for (int k : oldKeys) {
            if (kIdx < numberOfColumns) {
                newNode[kIdx] = k;
                kIdx += 2;
            }
        }

        writeToFile(file, nodeIndex, newNode);

        // Update parent separator to borrowedKey (the key we took from left)
        if (parentSepIdx < numberOfColumns) {
            parent[parentSepIdx] = borrowedKey;
        }
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

    // find left position in parent children
    vector<int> parentChildren = getChildren(parent);
    int leftPos = -1;
    for (int i = 0; i < parentChildren.size(); ++i)
        if (parentChildren[i] == leftIndex)
            leftPos = i;

    if (leftPos == -1) return;

    int sepIndex = 1 + 2 * leftPos; // parent separator key
    int separatorKey = parent[sepIndex];

    bool leftIsLeaf = (left[0] == 0);

    // ----------------------------------------------------------------------
    // CASE 1: LEAF MERGE
    // ----------------------------------------------------------------------
    if (leftIsLeaf) {
        int leftKeyCount = nodeKeyCount(left);
        int rightKeyCount = nodeKeyCount(right);

        // append all keys & ptrs from right to left
        for (int i = 0; i < rightKeyCount; ++i) {
            left[1 + 2 * (leftKeyCount + i)] = right[1 + 2 * i];
            left[2 + 2 * (leftKeyCount + i)] = right[2 + 2 * i];
        }

        // IMPORTANT: handle leaf next pointer (rightmost column)
        // last column assumed to be right pointer for leaf
        // if you are using the last child pointer as leaf-next:
        left[numberOfColumns - 1] = right[numberOfColumns - 1];

        writeToFile(file, leftIndex, left);
        deallocateNode(file, rightIndex);
    }

    // ----------------------------------------------------------------------
    // CASE 2: INTERNAL MERGE
    // ----------------------------------------------------------------------
    else {
        vector<int> leftChildren = getChildren(left);
        vector<int> rightChildren = getChildren(right);

        vector<int> leftKeys;
        vector<int> rightKeys;

        for (int k = 2; k < numberOfColumns; k += 2)
            if (left[k] != -1) leftKeys.push_back(left[k]);

        for (int k = 2; k < numberOfColumns; k += 2)
            if (right[k] != -1) rightKeys.push_back(right[k]);

        // new structure: interleave children & keys
        vector<int> newLeft(numberOfColumns, -1);
        newLeft[0] = 1; // internal node

        int cptr = 1; // child pointers at odd indices
        int kptr = 2; // keys at even indices

        // left children + left keys
        for (int i = 0; i < leftKeys.size(); ++i) {
            newLeft[cptr] = leftChildren[i];
            newLeft[kptr] = leftKeys[i];
            cptr += 2;
            kptr += 2;
        }

        // insert separator key
        newLeft[cptr] = leftChildren[leftKeys.size()];
        newLeft[kptr] = separatorKey;
        cptr += 2;
        kptr += 2;

        // right children + right keys
        for (int i = 0; i < rightKeys.size(); ++i) {
            newLeft[cptr] = rightChildren[i];
            newLeft[kptr] = rightKeys[i];
            cptr += 2;
            kptr += 2;
        }
        newLeft[cptr] = rightChildren[rightKeys.size()];

        writeToFile(file, leftIndex, newLeft);
        deallocateNode(file, rightIndex);
    }

    // ----------------------------------------------------------------------
    // REMOVE SEPARATOR KEY + RIGHT CHILD FROM PARENT
    // ----------------------------------------------------------------------
    // remove key at sepIndex AND child pointer for right sibling
    vector<int> updatedParent = parent;

    // shift keys & children left
    for (int i = sepIndex; i < numberOfColumns - 2; i += 2) {
        updatedParent[i] = updatedParent[i + 2];
        updatedParent[i + 1] = updatedParent[i + 3];
    }
    updatedParent[numberOfColumns - 2] = -1;
    updatedParent[numberOfColumns - 1] = -1;

    writeToFile(file, parentIndex, updatedParent);

    // ----------------------------------------------------------------------
    // HANDLE PARENT UNDERFLOW (if parent is not root)
    // ----------------------------------------------------------------------
    vector<int> parentNow(numberOfColumns);
    readFromFile(file, parentIndex, parentNow);

    if (parentIndex != 1 && nodeKeyCount(parentNow) < minimumKeys()) {
        int gp = findParent(file, parentIndex);
        if (gp != -1) {
            auto sibs = findNearestSiblings(file, parentIndex);
            bool fixed = false;

            if (sibs.first != -1) fixed = borrowFromLeft(file, parentIndex, sibs.first, gp);
            if (!fixed && sibs.second != -1) fixed = borrowFromRight(file, parentIndex, sibs.second, gp);

            if (!fixed) {
                if (sibs.first != -1)
                    mergeWithSiblingAndDeleteFromParent(file, sibs.first, parentIndex, gp);
                else if (sibs.second != -1)
                    mergeWithSiblingAndDeleteFromParent(file, parentIndex, sibs.second, gp);
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
///joe ehab (3eeeentil el flater 3 ra2i yousef huissen)
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

int  SearchARecord (char* filename, int RecordID) {
    fstream file(filename,ios::in|ios::binary);
    if(!file.is_open())
        cout<<"error while opening file ";

    int nodeIndex = 1;
    int node[numberOfColumns];
    while (true) {
        file.seekg(numberOfColumns * nodeIndex*sizeof(int),ios::beg);
        file.read((char*)node,numberOfColumns *sizeof(int));

        //the node is leaf if the frsit num on the array is 0
        if(node[0] == 0) {
            for(int i = 1;i < numberOfColumns; i += 2) {
                if(node[i] == RecordID)
                    return node[i+1];
            }
            return -1;
        }
        // non leave node
        else{
            int nextChild = -1;
            for(int i=2 ;i<numberOfColumns;i += 2) {
                int id = node[i];
                if(id == -1 || id > RecordID) {
                    nextChild = node[i - 1];
                    break;
                }
            }
            if (nextChild == -1) {
                nextChild = node[numberOfColumns - 1];
            }
            nodeIndex = nextChild;
        }
    }
    return -1;
}

int main() {

     char filename[] = "indexfile.bin";

    // CreateIndexFileFile(filename, 10, M);
    // DisplayIndexFileContent(filename);
    //
    // // FIRST BATCH
    // InsertNewRecordAtIndex(filename, 3, 12);
    // cout << "\nAfter inserting 3:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 7, 24);
    // cout << "\nAfter inserting 7:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 10, 48);
    // cout << "\nAfter inserting 10:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 24, 60);
    // cout << "\nAfter inserting 24:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 14, 72);
    // cout << "\nAfter inserting 14:\n";
    // DisplayIndexFileContent(filename);
    // // TRIGGER FIRST SPLIT
    // InsertNewRecordAtIndex(filename, 19, 84);
    // cout << "\nAfter inserting 19:\n";
    // DisplayIndexFileContent(filename);
    //
    // InsertNewRecordAtIndex(filename, 30, 96);
    // cout << "\nAfter inserting 30:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 15, 108);
    // cout << "\nAfter inserting 15:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 1, 120);
    // cout << "\nAfter inserting 1:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 5, 132);
    // cout << "\nAfter inserting 1:\n";
    // DisplayIndexFileContent(filename);
    //
    // // Trigger second split
    // InsertNewRecordAtIndex(filename, 2, 144);
    // cout << "\nAfter inserting 2:\n";
    // DisplayIndexFileContent(filename);
    // //third patch
    // InsertNewRecordAtIndex(filename, 8, 156);
    // cout << "\nAfter inserting 8:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 9, 168);
    // cout << "\nAfter inserting 9:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 6, 180);
    // cout << "\nAfter inserting 6:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 11, 192);
    // cout << "\nAfter inserting 11:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 12, 204);
    // cout << "\nAfter inserting 12:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 17, 216);
    // cout << "\nAfter inserting 17:\n";
    // DisplayIndexFileContent(filename);
    // InsertNewRecordAtIndex(filename, 18, 228);
    // cout << "\nAfter inserting 18:\n";
    // DisplayIndexFileContent(filename);
    // //final insert
    // InsertNewRecordAtIndex(filename, 32, 240);
    // cout << "\nAfter last insert 32:\n";
    DisplayIndexFileContent(filename);

    if(SearchARecord(filename,10) == -1) {
        cout<<"111";
    }
    DisplayIndexFileContent(filename);


    return 0;

}
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

void readFromFile(fstream& file,int nodeIndex,const vector<int>&row) {
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
    return descendant / 2;
}


int nodeKeyCount(const vector<int> &row){
    return numberOfKeys(row);
}

vector<pair<int, int>> getPairs(const vector<int> &node){
    vector<pair<int, int>> Pairs;
    for(int i = 1; i < numberOfColumns; i += 2){
        if(node[i] != -1){
            Pairs.emplace_back(node[i], node[i+1]);
        }
    }
    return Pairs;

}

void putPairs(vector<int> &node, const vector<pair<int, int>>& pairs, int type ){
    node[0] = type;
    for(int i = 1; i < numberOfColumns; i++) {
        node[i] = -1;
    }
    for(int i = 0; i < pairs.size(); i++){
        node[1 + (2 * i)] = pairs[i].first;
        node[2 + (2 * i)] = pairs[i].second;

    }
}

vector<int> getChildren(const vector<int> & node){
    vector<int> children;
    if(node[0] == 1){
        for(int i = 0; i < descendant; i++){
            children.emplace_back(node[1 + 2 * i]); 
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
    if(node[0] == 0){
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

    for(int i = 0; i < children.size(); i++ ){
        if(children[i] == nodeIndex){
            position = i;
            break;
        }
    }

    if(position == -1){
        return make_pair(-1,-1);
    }
    int left, right = -1;
    if (position > 0){
        left = children[position-1];
    }
    if(position < children.size() - 1){
        right = children[position + 1];
    }
    return make_pair(left, right);

}

void deallocateNode(fstream& file, int nodeIndex){
    vector<int> header(numberOfColumns);
    readFromFile(file, 0, header);

    int next = header[1];
    header[1] = nodeIndex;

    writeToFile(file, 0, header);
    vector<int> node(numberOfColumns, -1);
    node[1] = next;
    writeToFile(file, nodeIndex, node);
}


pair<int,int> findKeyNode(fstream& file, int recordID){
    int nodeIndex = 1;
    vector<int>node(numberOfColumns);
    readFromFile(file, nodeIndex, node);
    if (node[0] == -1){
        return make_pair(-1,-1);
    }

    /*
        i will travers every non leaf node until get the record 
        1- in for loop start from 1 beacuse node like this (child0(pointer), key0, child1(pointer), key1, ............. )
        2- in this loop i decide which node i will choice  so when  recordID <= node[i] hence i go in node[i-1] child
        3- if recordid greater than all keys then go in last child
        ------------------------------------
        in second for loop 
        1-We travers all the keys
        2-If we find key then We return the node number
        3- the key's location within it (using (i − 1) / 2, beacuse key number is odd so i will convert main loc in node to key number)
    */

    while (node[0] == 1) {  
    
    bool found = false;

    // Case 1: compare with first key (key0 at index 2)
    if (node[2] != -1 && recordID < node[2]) {
        nodeIndex = node[1];   // child0
        found = true;
    } 
    else {
        // Case 2: compare with remaining keys
        for (int i = 4; i < numberOfColumns; i += 2) {
            if (node[i] != -1 && recordID < node[i]) {
                int childIdx = node[i - 1];
                nodeIndex = childIdx;
                found = true;
                break;
            }
        }
    }

    //Case 3: larger than all keys  go to last child
    if (!found) {
        nodeIndex = node[numberOfColumns - 1]; 
    }

    // Load next node
    readFromFile(file, nodeIndex, node);
    }

    for(int i = 1; i < numberOfColumns; i += 2){
        if(node[i] == recordID){
            return make_pair(nodeIndex, ((i-1) / 2));
        }
    }

    return make_pair(-1,-1);
}



 void removeAtPosition(fstream& file, int nodeIndex, int position) {
    vector<int> node(numberOfColumns);
    readFromFile(file, nodeIndex, node);

    bool isLeaf = (node[0] == 0);

    if (isLeaf) {
        // Shift keys and pointers in leaf
        for (int i = position; i < descendant - 1; i++) {
            node[1 + 2 * i] = node[1 + 2 * (i + 1)];
            node[2 + 2 * i] = node[2 + 2 * (i + 1)];
        }
        // Clear last 
        node[1 + 2 * (descendant - 1)] = -1;
        node[2 + 2 * (descendant - 1)] = -1;
    } 
    else {
        // Internal node: remove key and its right child
        for (int i = position; i < descendant - 1; i++) {
            node[2 + 2 * i] = node[2 + 2 * (i + 1)];  // shift keys
            node[1 + 2 * (i + 1)] = node[1 + 2 * (i + 2)]; // shift children
        }
        // Clear last key and child
        node[2 + 2 * (descendant - 1)] = -1;
        node[numberOfColumns - 1] = -1;
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
    if (rightKeys <= minKeys) {
        return false;
    }

    bool isLeaf = (node[0] == 0);

    if (isLeaf) {
        // Borrow first key from right
        int borrowedKey = right[1];
        int borrowedPtr = right[2];

        // Shift right node keys/pointers left
        for (int i = 0; i < descendant - 1; i++) {
            right[1 + 2 * i] = right[1 + 2 * (i + 1)];
            right[2 + 2 * i] = right[2 + 2 * (i + 1)];
        }
        right[1 + 2 * (descendant - 1)] = -1;
        right[2 + 2 * (descendant - 1)] = -1;
        writeToFile(file, rightIndex, right);

        // Insert borrowed key at correct position in current node
        int position = 0;
        while (position < descendant && node[1 + 2 * position] != -1 && node[1 + 2 * position] < borrowedKey) {
            position++;
        }
        // Shift node keys/pointers right to make space
        for (int i = descendant - 1; i > position; i--) {
            node[1 + 2 * i] = node[1 + 2 * (i - 1)];
            node[2 + 2 * i] = node[2 + 2 * (i - 1)];
        }

        node[1 + 2 * position] = borrowedKey;
        node[2 + 2 * position] = borrowedPtr;
        writeToFile(file, nodeIndex, node);

        // Update parent's separator key
        vector<int> children = getChildren(parent);
        int parentPosition = -1;
        for (int i = 0; i < children.size(); i++){ 
            if (children[i] == nodeIndex) {
                parentPosition = i;
                break;
            }
        }

        if (parentPosition >= 0 && parentPosition < children.size() - 1) {
            parent[1 + 2 * parentPosition] = right[1];
            writeToFile(file, parentIndex, parent);
        }

        return true;
    } 
    else {
        // Internal node: borrow leftmost key/child from right
        int borrowedKey = right[2];     // first key
        int borrowedChild = right[1];   // first child

        // Shift right node keys/children left
        for (int i = 0; i < descendant - 1; i++) {
            right[1 + 2 * i] = right[1 + 2 * (i + 1)];
            right[2 + 2 * i] = right[2 + 2 * (i + 1)];
        }
        right[1 + 2 * (descendant - 1)] = -1;
        right[2 + 2 * (descendant - 1)] = -1;
        writeToFile(file, rightIndex, right);

        // Append parent's separator key to current node
        vector<int> oldChildren = getChildren(node);
        vector<int> oldKeys;
        for (int k = 2; k < numberOfColumns; k += 2) {
            if (node[k] != -1) {
                oldKeys.push_back(node[k]);
            }
        }

        int parentPosition = -1;
        vector<int> parentChildren = getChildren(parent);
        for (int i = 0; i < parentChildren.size(); i++){
             if (parentChildren[i] == nodeIndex) { 
                parentPosition = i; 
                break; 
            }
        }
           
        int parentKey = parent[1 + 2 * parentPosition];
        oldKeys.push_back(parentKey); // parent's key goes to current node

        // Rebuild node
        vector<int> newNode(numberOfColumns, -1);
        newNode[0] = 1; // internal

        // set children
        int childIndex = 1;
        for (int c : oldChildren) { 
            newNode[childIndex] = c; 
            childIndex += 2; 
        }
        if (childIndex < numberOfColumns) {
            newNode[childIndex] = borrowedChild;
        }

        // set keys
        int keyIndex = 2;
        for (int k : oldKeys) { 
            newNode[keyIndex] = k;
            keyIndex += 2; 
        }

        writeToFile(file, nodeIndex, newNode);

        // Update parent's separator key to borrowed key
        parent[1 + 2 * parentPosition] = borrowedKey;
        writeToFile(file, parentIndex, parent);

        return true;
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
    // try borrow
    if (sibs.first != -1) {
        // (ismail)
        //fixed = borrowFromLeft(file, foundNode, sibs.first, parent);
    }
    if (!fixed && sibs.second != -1) {
        fixed = borrowFromRight(file, foundNode, sibs.second, parent);
    }
    if (!fixed) {
        // merge  // (ismail)
        if (sibs.first != -1) {
            //mergeWithSiblingAndDeleteFromParent(file, foundNode, sibs.first, parent);
        } 
        else if (sibs.second != -1) {
            //mergeWithSiblingAndDeleteFromParent(file, foundNode, sibs.second, parent);
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
    };

    char name[fileNameSize] = "BTreeIndex.bin";
    createIndexFile(name,numberOfRecords,descendant);
    displayIndexFileContent(name);
    cout << "----------------------------------" << endl;
    for(int i = 0;i<5;i++){
        insertNewRecordAtIndex(name,arr[i].first,arr[i].second);
        displayIndexFileContent(name);
        cout << "----------------------------------" << endl;
    }

    return 0;
}

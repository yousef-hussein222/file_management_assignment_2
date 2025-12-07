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
        cout<< "Error happen when open index file...!" << endl;
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

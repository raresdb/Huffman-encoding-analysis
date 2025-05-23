//
// Created by zawawy on 12/22/18.
//
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <bits/stdc++.h>
#include <mpi.h>
#include "Node.h"
using namespace std;

#ifndef HUFFMANCODING_HUFFER_H
#define HUFFMANCODING_HUFFER_H

#define CHUNK_SIZE 10000

class Huffman {

    unordered_map<char,string> codeMap;
    void encodeCharacters(Node * rootNode, string codeString);
    void writeHeader(ofstream &outputStream);
    void readHeader(ifstream &inputStream);
    Node* buildDecodingTree(unordered_map<char,string> encodingMap);
    class myCompartor{
    public :
        int operator() (Node* node1, Node* node2);
    };
public :
    void huffer(unordered_map<char, int> frequencyMap);
    void deHuffer(string fileName,string decompressedFileName);
    void compressTofile(string InputfileName ,string OutputfileName);
    void decompressToFile(Node *rootNode, ifstream &inputStream, string decompressedFileName);
    unordered_map<char,string>* getCodeMap();
};

#endif //HUFFMANCODING_HUFFER_H

//
// Created by zawawy on 12/22/18.
//

#include "Huffman.h"

#define INTERNAL_NODE_CHARACTER char(128)
#define PSEUDO_EOF char(129)
#define CHARACTER_CODE_SEPERATOR char(130)
#define HEADER_ENTRY_SEPERATOR char(131)
#define HEADER_TEXT_SEPERATOR char(132)

int Huffman::myCompartor::operator()(Node *node1, Node *node2) {
    return node1->getFrequency() > node2->getFrequency();
}

void Huffman::huffer(unordered_map<char, int> frequencyMap) {
    priority_queue <Node *, vector<Node *>, myCompartor> HufferQueue;
    string tempString;
    Node *leftNode, *rightNode, *newNode;
    for (const auto &item : frequencyMap)
        HufferQueue.push(new Node(item.first, item.second));
    HufferQueue.push(new Node(PSEUDO_EOF, 1));
    while (HufferQueue.size() != 1) {
        leftNode = HufferQueue.top();
        HufferQueue.pop();
        rightNode = HufferQueue.top();
        HufferQueue.pop();
        newNode = new Node(INTERNAL_NODE_CHARACTER, leftNode->getFrequency() + rightNode->getFrequency());
        HufferQueue.push(newNode);
        newNode->setLeft(leftNode);
        newNode->setRight(rightNode);
    }
    encodeCharacters(HufferQueue.top(), tempString);
}

void Huffman::encodeCharacters(Node *rootNode, string codeString) {

    if (!rootNode)
        return;
    if (rootNode->getCharacter() != INTERNAL_NODE_CHARACTER) {
        codeMap[rootNode->getCharacter()] = codeString;

    }
    encodeCharacters(rootNode->getLeft(), codeString + "0");
    encodeCharacters(rootNode->getRight(), codeString + "1");
}

void Huffman::compressTofile(string InputfileName ,string OutputfileName) {
    char character;
    string file;
    ifstream inputStream;
    ofstream outputStream;
    outputStream.open(OutputfileName, ios::out);
    inputStream.open(InputfileName, ios::in);
    writeHeader(outputStream);
    while (inputStream.get(character)) {
        file += codeMap[character];
        if (file.length() > 8) {
            bitset<8> bits(file.substr(0, 8));
            file.erase(0, 8);
            char c = char(bits.to_ulong());
            outputStream << c;
        }
    }
    inputStream.close();
    file += codeMap[PSEUDO_EOF];
    unsigned long remainder = (file.size() - 1) % 8;
    for (int i = 0; i < 8 - remainder; ++i)
        file += '0';
    stringstream stringStream(file);

    while (stringStream.good()) {
        bitset<8> bits;
        stringStream >> bits;
        char c = char(bits.to_ulong());
        outputStream << c;
    }


    outputStream.flush();
    outputStream.close();
}

void Huffman::writeHeader(ofstream &outputStream) {
    for (const auto &item : codeMap)
        outputStream << item.first << CHARACTER_CODE_SEPERATOR << item.second << HEADER_ENTRY_SEPERATOR;
    outputStream << HEADER_TEXT_SEPERATOR;
}

void Huffman::deHuffer(string compressedFileName, string decompressedFileName) {
    char character;
    string codeString;
    ifstream inputStream;
    inputStream.open(compressedFileName, ios::in);
    readHeader(inputStream);
    Node *rootNode = buildDecodingTree(codeMap);
    decompressToFile(rootNode, inputStream, decompressedFileName);
}

void Huffman::readHeader(ifstream &inputStream) {
    codeMap.clear();
    char character;
    inputStream.get(character);
    char key = character;
    while (character != HEADER_TEXT_SEPERATOR) {
        if (character == CHARACTER_CODE_SEPERATOR) {
            inputStream.get(character);
            while (character != HEADER_ENTRY_SEPERATOR) {
                codeMap[key] += character;
                inputStream.get(character);
            }
        } else
            key = character;
        inputStream.get(character);
    }
}

Node *Huffman::buildDecodingTree(unordered_map<char, string> encodingMap) {
    Node *rootNode = new Node(INTERNAL_NODE_CHARACTER);
    Node *previousNode;

    for (const auto &item : encodingMap) {
        previousNode = rootNode;
        Node *newNode = new Node(item.first);
        string characterCode = item.second;
        for (int i = 0; i < characterCode.size(); ++i) {
            if (characterCode[i] == '0') {
                if (i == characterCode.size() - 1)
                    previousNode->setLeft(newNode);
                else {
                    if (!previousNode->getLeft()) {
                        previousNode->setLeft(new Node(INTERNAL_NODE_CHARACTER));
                        previousNode = previousNode->getLeft();
                    } else previousNode = previousNode->getLeft();
                }
            } else {
                if (i == characterCode.size() - 1)
                    previousNode->setRight(newNode);
                else {
                    if (!previousNode->getRight()) {
                        previousNode->setRight(new Node(INTERNAL_NODE_CHARACTER));
                        previousNode = previousNode->getRight();
                    } else previousNode = previousNode->getRight();
                }
            }
        }

    }
    return rootNode;
}

void Huffman::decompressToFile(Node *rootNode, ifstream &inputStream, string decompressedFileName) {
    ofstream outputStream;
    char character;

    outputStream.open(decompressedFileName, ios::out);
    Node *traversingPointer = rootNode;
    string bit_string;
    while (inputStream.get(character)) {
        bitset<8> bits(character);
        bit_string += bits.to_string();
        int bit_pos = 0;
        while (bit_pos < bit_string.length()) {
            if (bit_string[bit_pos] == '0') {
                traversingPointer = traversingPointer->getLeft();
            } else {
                traversingPointer = traversingPointer->getRight();
            }

            if (traversingPointer->getCharacter() != INTERNAL_NODE_CHARACTER) {
                if (traversingPointer->getCharacter() == PSEUDO_EOF) {
                    break;
                }
                outputStream << traversingPointer->getCharacter();
                traversingPointer = rootNode;
            }

            bit_pos++;
        }
        if (traversingPointer->getCharacter() != INTERNAL_NODE_CHARACTER) {
            if (traversingPointer->getCharacter() == PSEUDO_EOF) {
                break;
            }
            outputStream << traversingPointer->getCharacter();
            traversingPointer = rootNode;
        }
        bit_string.clear();
    }
    outputStream.flush();
    outputStream.close();
}

//
// Created by zawawy on 12/22/18.
//

#include "Huffman.h"
#include "omp.h"

#define INTERNAL_NODE_CHARACTER char(128)
#define PSEUDO_EOF char(129)
#define CHARACTER_CODE_SEPERATOR char(130)
#define HEADER_ENTRY_SEPERATOR char(131)
#define HEADER_TEXT_SEPERATOR char(132)

#define CHUNK_SIZE 10000

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

void Huffman::compressTofile(string InputfileName, string OutputfileName) {
    ifstream inputStream;
    ofstream outputStream;
    outputStream.open(OutputfileName, ios::out);
    inputStream.open(InputfileName, ios::in);
    writeHeader(outputStream);

    string accumulator;

    #pragma omp parallel num_threads(8)
    {
        #pragma omp single nowait
        {
            int num_threads = omp_get_num_threads();

            char *line;
            char *preloaded = new char[CHUNK_SIZE * num_threads];
            inputStream.read(preloaded, CHUNK_SIZE * num_threads);
            int char_count = inputStream.gcount();
            string *res[num_threads];
            string res_bin[num_threads];
            for (int i = 0; i < num_threads; ++i) {
                res[i] = NULL;
            }

            while(inputStream || char_count) {
                line = preloaded;

                res[0] = new string(accumulator);
                accumulator.clear();

                for (int i = 0; i < num_threads; ++i) {
                    int task_pos = i;
                    #pragma omp task default(none) firstprivate(task_pos, char_count) shared(line, res)
                    {
                        if (!res[task_pos]) {
                            res[task_pos] = new string();
                        }
                        int start = task_pos * CHUNK_SIZE;
                        int stop = min((task_pos + 1) * CHUNK_SIZE, char_count);

                        for (int cpos = start; cpos < stop; ++cpos) {
                            *res[task_pos] += codeMap[line[cpos]];
                        }
                    }
                }

                if (inputStream) {
                    preloaded = new char[CHUNK_SIZE * num_threads];
                    inputStream.read(preloaded, CHUNK_SIZE * num_threads);
                    char_count = inputStream.gcount();
                } else {
                    char_count = 0;
                }

                #pragma omp taskwait

                delete line;

                for (int i = 0; i < num_threads; ++i) {
                    int remainder = res[i]->length() % 8;

                    if (i + 1 < num_threads) {
                        res[i + 1]->insert(0,
                            res[i]->substr(res[i]->length() - remainder, remainder)
                        );
                    } else {
                        accumulator.assign(res[i]->substr(res[i]->length() - remainder, remainder));
                    }
                }

                for (int i = 0; i < num_threads; ++i) {
                    #pragma omp task firstprivate(i) shared(res_bin, res)
                    {
                        string::iterator pos_it = res[i]->begin();
                        string::iterator end_it = res[i]->end();

                        for(pos_it; end_it - pos_it >= 8; pos_it += 8) {
                            bitset<8> bits(string(pos_it, pos_it + 8));
                            char character = char(bits.to_ulong());
                            res_bin[i] += character;
                        }
                        delete res[i];
                        res[i] = NULL;
                    }
                }

                #pragma omp taskwait

                for (int i = 0; i < num_threads; ++i) {
                    outputStream << res_bin[i];
                    res_bin[i].clear();
                }
            }
            inputStream.close();
            accumulator += codeMap[PSEUDO_EOF];
            unsigned long remainder = (accumulator.size()) % 8;
            for (int i = 0; i < 8 - remainder; ++i)
                accumulator += '0';

            for (int i = 0; i < accumulator.length(); i += 8) {
                bitset<8> bits(accumulator.substr(i, 8));
                char c = char(bits.to_ulong());
                outputStream << c;
            }

            outputStream.flush();
            outputStream.close();
        }
    }
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

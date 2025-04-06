//
// Created by zawawy on 12/22/18.
//

#include "Huffman.h"

#define INTERNAL_NODE_CHARACTER char(128)
#define PSEUDO_EOF char(129)
#define CHARACTER_CODE_SEPERATOR char(130)
#define HEADER_ENTRY_SEPERATOR char(131)
#define HEADER_TEXT_SEPERATOR char(132)

unsigned thread_count;

void* compressTofileWork(void* args) {
    compressTofileWork_args* args_struct = (compressTofileWork_args*)args;

    do {
        sem_wait(args_struct->sem);
        args_struct->result.clear();
        args_struct->result_bin.clear();

        if(!args_struct->line)
            break; 

        unsigned end = (CHUNK_SIZE * (args_struct->id + 1)) < args_struct->line_size ?
            (CHUNK_SIZE * (args_struct->id + 1)) : args_struct->line_size;

        for(unsigned idx = CHUNK_SIZE * args_struct->id; idx < end; idx++) {
            args_struct->result += args_struct->huffman->getCodeFromMap((*args_struct->line)[idx]);
        }

        pthread_barrier_wait(args_struct->barr);
        sem_wait(args_struct->sem);

        string::iterator pos_it = args_struct->result.begin();
        string::iterator end_it = args_struct->result.end();

        for(pos_it; end_it - pos_it >= 8; pos_it += 8) {
            bitset<8> bits(string(pos_it, pos_it + 8));
            char character = char(bits.to_ulong());
            args_struct->result_bin.push_back(character);
        }

        pthread_barrier_wait(args_struct->barr);
    } while(true);

    return NULL;
}

string Huffman::getCodeFromMap(char character) {
    return codeMap[character];
}

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
    char *line;
    char *preloaded = new char[CHUNK_SIZE * thread_count];
    ifstream inputStream;
    ofstream outputStream;
    pthread_t threads[thread_count];
    pthread_barrier_t barr;
    sem_t sem;
    compressTofileWork_args thread_args[thread_count];
    string rest;

    outputStream.open(OutputfileName, ios::out);
    inputStream.open(InputfileName, ios::in);
    writeHeader(outputStream);
    
    sem_init(&sem, 0, 0);
    pthread_barrier_init(&barr, NULL, thread_count + 1);

    for(unsigned i = 0; i < thread_count; i++) {
        thread_args[i] = {huffman: this, barr: &barr, sem: &sem, id: i, line: &line};
        pthread_create(&threads[i], NULL, compressTofileWork, &thread_args[i]);
    }

    inputStream.read(preloaded, CHUNK_SIZE * thread_count);
    int line_size = inputStream.gcount();

    while (inputStream || line_size) {
        line = preloaded;

        for(unsigned i = 0; i < thread_count; i++) {
            thread_args[i].line_size = line_size;
        }

        for(unsigned i = 0; i < thread_count; i++) {
            sem_post(&sem);
        }

        if (inputStream) {
            preloaded = new char[CHUNK_SIZE * thread_count];
            inputStream.read(preloaded, CHUNK_SIZE * thread_count);
            line_size = inputStream.gcount();
        } else {
            line_size = 0;
        }
        
        pthread_barrier_wait(&barr);
        thread_args[0].result.insert(0, rest);
        rest.clear();

        delete line;
        
        for(unsigned i = 0; i < thread_count; i++) {
            int remainder = thread_args[i].result.size() % 8;

            if(i < thread_count - 1) {
                thread_args[i + 1].result.insert(0,
                    thread_args[i].result.substr(thread_args[i].result.size() - remainder, remainder));
            } else {
                rest.assign(thread_args[i].result.substr(thread_args[i].result.size() - remainder, remainder));
            }
        }

        for(unsigned i = 0; i < thread_count; i++) {
            sem_post(&sem);
        }

        pthread_barrier_wait(&barr);

        for(unsigned i = 0; i < thread_count; i++) {
            outputStream << thread_args[i].result_bin;
        }
    }

    inputStream.close();

    for(unsigned i = 0; i < thread_count; i++) {
       thread_args[i].line = NULL;
    }

    for(unsigned i = 0; i < thread_count; i++) {
        sem_post(&sem);
    }

    for(unsigned i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&sem);
    pthread_barrier_destroy(&barr);

    rest.append(codeMap[PSEUDO_EOF]);
    unsigned long remainder = (rest.size()) % 8;

    for(unsigned i = 0; i < 8 - remainder; i++) {
        rest += '0';
    }

    string::iterator it = rest.begin();
    string::iterator end = rest.end();

    for(it; it != end; it += 8) {
        bitset<8> bits(string(it, it + 8));
        char character = char(bits.to_ulong());
        outputStream << character;
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

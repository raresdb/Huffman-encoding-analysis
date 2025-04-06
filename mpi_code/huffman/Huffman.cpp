//
// Created by zawawy on 12/22/18.
//

#include "Huffman.h"

#define INTERNAL_NODE_CHARACTER char(128)
#define PSEUDO_EOF char(129)
#define CHARACTER_CODE_SEPERATOR char(130)
#define HEADER_ENTRY_SEPERATOR char(131)
#define HEADER_TEXT_SEPERATOR char(132)

unsigned procs_count;
int proc_rank;

void ring_share(void* buff, int &count, int max_count) {
    int gap = 1;
    int recv;
    int tmp_count; 
    MPI_Status status;

    while(gap <= procs_count) {
		for(int snd = procs_count - gap; snd > 0; snd -= 2 * gap) {
            if((procs_count - snd) / gap % 2) {
                recv = (snd - gap) > 0 ? snd - gap : 0;

            if(snd == recv)
                continue;

                if(proc_rank == snd)
                    MPI_Send(buff , count , MPI_CHARACTER , recv , 0 , MPI_COMM_WORLD);
                else if(proc_rank == recv) {
                    MPI_Recv((char*)buff + count, max_count , MPI_CHARACTER , snd , 0 , MPI_COMM_WORLD , &status);
                    MPI_Get_count(&status , MPI_CHARACTER , &tmp_count);
                    count += tmp_count;
                }
            }
        }
		
		gap *= 2;
	}
}

unordered_map<char,string>* Huffman::getCodeMap() {
    return &codeMap;
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
    unsigned line_size;
    int result_bin_size;
    char result_bin[CHUNK_SIZE * (procs_count - 1)];
    char rest[16];
    MPI_Status status;

    rest[0] = 0;

    if(!proc_rank) {
        ifstream inputStream;
        ofstream outputStream;
        char line[CHUNK_SIZE * (procs_count - 1)];

        outputStream.open(OutputfileName, ios::out);
        inputStream.open(InputfileName, ios::in);
        writeHeader(outputStream);

        while(inputStream) {
            inputStream.read(line, CHUNK_SIZE * (procs_count - 1));
            line_size = inputStream.gcount();

            for(int i = 1; i < procs_count; i++) {
                int proc_line_size = min(CHUNK_SIZE, max(0, (int)line_size - (i - 1) * CHUNK_SIZE));
                MPI_Send((void*)&line[CHUNK_SIZE * (i - 1)], proc_line_size , MPI_CHARACTER , i , 0 , MPI_COMM_WORLD);
            }

            MPI_Send((void*)rest , strlen(rest) + 1 , MPI_CHARACTER , 1 , 0 , MPI_COMM_WORLD);
            MPI_Recv((void*)rest , 16 , MPI_CHARACTER , procs_count - 1 , 0 , MPI_COMM_WORLD , NULL);

            result_bin_size = 0;
            ring_share(result_bin, result_bin_size, CHUNK_SIZE * (procs_count - 1));
            outputStream.write(result_bin, result_bin_size);
        }

        inputStream.close();

        char pseudo_eof = PSEUDO_EOF;

        for(auto i = 1; i < procs_count; i++)
            MPI_Send((void*)&pseudo_eof, 1 , MPI_CHARACTER , i , 0 , MPI_COMM_WORLD);

        strcat(rest, codeMap[PSEUDO_EOF].c_str());
        unsigned long remainder = (strlen(rest)) % 8;

        unsigned rest_len = strlen(rest);

        for(unsigned i = 0; i < 8 - remainder; i++)
            rest[rest_len + i] += '0';

        rest[rest_len + 8 - remainder] = 0;

        for(unsigned idx = 0; idx < strlen(rest); idx += 8) {
            bitset<8> bits(string(rest), idx, 8);
            char character = char(bits.to_ulong());
            outputStream << character;
        }

        outputStream.flush();
        outputStream.close();
    }

    if(proc_rank) {
        char line[CHUNK_SIZE];
        char result[8 * CHUNK_SIZE];

        MPI_Recv((void*)line , CHUNK_SIZE , MPI_CHARACTER , 0 , 0 , MPI_COMM_WORLD , &status);
        MPI_Get_count(&status , MPI_CHARACTER , (int*)&line_size);
        
        while(line[0] != PSEUDO_EOF) {
            result[0] = 0;

            for(unsigned idx = 0; idx < line_size; idx++) {
                strcat(result, codeMap[line[idx]].c_str());
            }

            MPI_Recv((void*)rest , 16 , MPI_CHARACTER , MPI_ANY_SOURCE , 0 , MPI_COMM_WORLD , NULL);
            char *tmp = strdup(result);
            strcpy(result, rest);
            strcat(result, tmp);
            free(tmp);
            unsigned rem = strlen(result) % 8;
            strcpy(rest, &result[strlen(result) - rem]);
            MPI_Send((void*)rest, strlen(rest) + 1 , MPI_CHARACTER, ((proc_rank < procs_count - 1) ? proc_rank + 1 : 0) , 0 , MPI_COMM_WORLD);
            result_bin_size = 0;

            for(int idx = 0; strlen(result) - idx >= 8; idx += 8) {
                bitset<8> bits(string(result), idx, 8);
                char character = char(bits.to_ulong());
                result_bin[result_bin_size++] = character;
            }

            ring_share(result_bin, result_bin_size, CHUNK_SIZE * (procs_count - 1));
            MPI_Recv((void*)line , CHUNK_SIZE , MPI_CHARACTER , 0 , 0 , MPI_COMM_WORLD , &status);
            MPI_Get_count(&status , MPI_CHARACTER , (int*)&line_size);
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

#include "huffman/FrequencyCounter.h"
#include "huffman/Huffman.h"
#include <chrono>

std::ifstream::pos_type filesize(std::string filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

void usage() {
    std::cout << "Usage: ./huff input_file output_file";
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        usage();
        exit(-1);
    }

    FrequencyCounter frequencyCounter;
    Huffman huffman;

    std::string inputFileName = argv[1], outputFileName = argv[2];

    clock_t tStart = clock();

    frequencyCounter.readFile(inputFileName);
    huffman.huffer(frequencyCounter.getFrequencyMap());

    auto start = std::chrono::high_resolution_clock::now();

    huffman.compressTofile(inputFileName, outputFileName);

    auto stop = std::chrono::high_resolution_clock::now();

    auto delta = std::chrono::duration<double>(stop - start).count();

    cout << "Compress time: " << delta << "sec" << '\n'
         << "Input File Size : " << filesize(inputFileName) << " bytes." << '\n'
         << "Compressed File Size : " << filesize(outputFileName) << " bytes." << '\n'
         << "Compression Ratio : " << (1.0 * filesize(outputFileName) / filesize(inputFileName)) << '\n';

    return 0;
}
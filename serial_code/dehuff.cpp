#include "huffman/FrequencyCounter.h"
#include "huffman/Huffman.h"

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

    Huffman huffman;

    std::string inputFileName = argv[1], outputFileName = argv[2];

    clock_t tStart = clock();

    huffman.deHuffer(inputFileName, outputFileName);

    cout << "Time taken: " << (1.0 * (clock() - tStart) / CLOCKS_PER_SEC) << "sec" << '\n'
         << "Input File (Compressed) Size : " << filesize(inputFileName) << " bytes." << '\n'
         << "DeCompressed File Size : " << filesize(outputFileName) << " bytes." << '\n';

    return 0;
}
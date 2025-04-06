#include "huffman/FrequencyCounter.h"
#include "huffman/Huffman.h"

extern unsigned procs_count;
extern int proc_rank;

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

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, (int*)&procs_count);
    MPI_Comm_rank(MPI_COMM_WORLD , &proc_rank);

    Huffman huffman;
    std::string inputFileName = argv[1], outputFileName = argv[2];
    pair<char, char[9]> entry_buff;

    if(!proc_rank) {
        time_t tStart = time(NULL);
        FrequencyCounter frequencyCounter;
        frequencyCounter.readFile(inputFileName);
        huffman.huffer(frequencyCounter.getFrequencyMap());

        for(auto entry: *huffman.getCodeMap()) {
            entry_buff.first = entry.first;
            strcpy(entry_buff.second, entry.second.c_str());
            MPI_Bcast((void*)&entry_buff.first , 1 , MPI_CHARACTER , 0 , MPI_COMM_WORLD);
            MPI_Bcast((void*)entry_buff.second , 9 , MPI_CHARACTER , 0 , MPI_COMM_WORLD);
        }

        entry_buff.first = char(0);
        entry_buff.second[0] = 0;
        MPI_Bcast((void*)&entry_buff.first , 1 , MPI_CHARACTER , 0 , MPI_COMM_WORLD);
        MPI_Bcast((void*)entry_buff.second , 1 , MPI_CHARACTER , 0 , MPI_COMM_WORLD);
        huffman.compressTofile(inputFileName, outputFileName);

        cout << "Time taken: " << (time(NULL) - tStart) << "sec" << '\n'
             << "Input File Size : " << filesize(inputFileName) << " bytes." << '\n'
             << "Compressed File Size : " << filesize(outputFileName) << " bytes." << '\n'
             << "Compression Ratio : " << (1.0 * filesize(outputFileName) / filesize(inputFileName)) << '\n';
    }

    if(proc_rank) {
        MPI_Bcast((void*)&entry_buff.first , 1 , MPI_CHARACTER , 0 , MPI_COMM_WORLD);
        MPI_Bcast((void*)entry_buff.second , 9 , MPI_CHARACTER, 0 , MPI_COMM_WORLD);

        while(strlen(entry_buff.second)) {
            pair<char, string> entry;
            entry.first = entry_buff.first;
            entry.second = string(entry_buff.second);
            huffman.getCodeMap()->insert(entry);
            MPI_Bcast((void*)&entry_buff.first , 1 , MPI_CHARACTER , 0 , MPI_COMM_WORLD);
            MPI_Bcast((void*)entry_buff.second , 9 , MPI_CHARACTER , 0 , MPI_COMM_WORLD);
        }

        huffman.compressTofile(inputFileName, outputFileName);
    }

    MPI_Finalize();

    return 0;
}
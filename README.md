# Huffman-encoding-analysis

The compress function for the Huffman encoding has been parallelised using 3 different technologies: pthreads, openmp and MPI.
You can generate a random file with the usage of the command: ./serial_code/test/generate_input_files.sh file_name size(in MB).

The algorithm used for parallelisation follows a master-slave model in which the main thread reads CHUNK_SIZE * num_threads
characters from the file and then distributes these chunks to the slaves. While each task processes their chunk, the main thread
reads the next bytes from the file in order to not waste time later with that. After each task ends its job, the main thread aligns
each processed chunk at 8 bytes. Other tasks are created to generate the final output and the main thread is tasked with writting the
output in the output file.

Analysys:
Using MPI, the runtime exceeded the runtime for the serial code by a ration of 2-3 to 1.
The performances of the pthreads and openmp implementations were similar.

serial: 33s
parallel:
	2  threads: 29.9 s
	4  threads: 19.6 s
	8  threads: 11.5 s
	16 threads: 8 s
	20 threads: 6.5 s

Conclusion:
pthreads, openmp >>>>>> mpi

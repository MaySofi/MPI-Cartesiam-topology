MPI Cartesian topology
Each process searches if the given string SubString presents into the current string.
Each process sends the characters from odd places of its current string to the left process and the even located characters to the up process.
The characters received from the right process became a first part of the current string,
The characters received from the down process became a second part of the current string.

Run C file in terminal:
Open terminal
cd /path/to/your_repository
mpicc HW02.c -o HW02
mpiexec -np 4 ./HW02

According to my text file, number of processes is 4
(It will work with any text file)

Output:
If the substring found, process 0 print all strings
else, process 0 print: "The string was not found"


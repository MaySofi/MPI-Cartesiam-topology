#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_NAME "data.txt"
#define DIM 2
#define BUFFER_SIZE 100

typedef enum {even, odd} evenOdd;
typedef enum {notFound, found} flagSubString;

char** readFromFile(const char* fileName, int* K, int *N, int* maxIterations, char** subString) {
	FILE* fp;
	char** data;
	int numberOfStrings;

	// Open file for reading data
	if ((fp = fopen(fileName, "r")) == 0) {
		printf("cannot open file %s for reading\n", fileName);
		exit(0);
	}

	// Number of string is K*K (==number of processes)
	fscanf(fp, "%d\t%d\t%d", K, N, maxIterations);
	numberOfStrings = (*K)*(*K);

	// SubString
	*subString = (char*)malloc((2 * (*N) +1)*sizeof(char));
	fscanf(fp, "%s", *subString);

	// Allocate array of points end Read data from the file
	data = (char**)malloc(numberOfStrings * sizeof(char*));
	if (data == NULL) {
		printf("Problem to allocate memory\n");
		exit(0);
	}

	for (int i = 0; i < numberOfStrings; i++)
	{
		*(data+i) = (char*)malloc((2 * (*N) +1)*sizeof(char));
		fscanf(fp, "%s", *(data + i));
		if (*(data+i) == NULL) {
			printf("Problem to allocate memory\n");
			exit(0);
		}
	}

	fclose(fp);
	return data;
}

char* packData(char** data, int dataSize, int stringLength)
{
	// dataSize	==	Number of string is K*K
	// stringLength	==	N*2 is the string length
	//data
	int position = 0;
	int packedDataSize;
	char* packedData;

	packedDataSize = dataSize * (stringLength + 1);
	packedData = (char*)malloc(packedDataSize * sizeof(char));
	if(packedData == NULL)
	{
		printf("Problem to allocate memory\n");
		exit(0);
	}

	for(int i = 0; i < dataSize; i++)
		MPI_Pack(*(data + i), stringLength+1, MPI_CHAR, packedData, packedDataSize, &position, MPI_COMM_WORLD);

	return packedData;
}


char* substringOddEven(const char *src, int len, evenOdd check)
{
	int j = check;
	char *dest = (char*)malloc((len+1)*sizeof(char));
	if(dest == NULL)
	{
		printf("Problem to allocate memory\n");
		exit(0);
	}

	for (int i = 0; i < len && (*(src + j) != '\0'); i++, j+=2)
	{
		*dest = *(src + j);
		dest++;
	}

	*dest = '\0';

	return dest - len;
}

char* concat(const char *s1, const char *s2)
{
	int len1 = strlen(s1);
	int len2 = strlen(s2);
	char *result = malloc((len1 + len2 + 1) * sizeof(char));

	memcpy(result, s1, len1);
	if(result == NULL)
	{
		printf("Problem to allocate memory\n");
		exit(0);
	}

	memcpy(result + len1, s2, len2 + 1); // +1 to copy the null-terminator
	if(result == NULL)
	{
		printf("Problem to allocate memory\n");
		exit(0);
	}

	return result;
}

int main(int argc, char* argv[])
{
	int rank, size, master = 0;
	int numberOfStrings, K, N, maxIterations, position = 0;
	int row, column;
	int max, source, dest;
	int dim[DIM], period[DIM], reorder;
	int coord[DIM];
	char buffer[BUFFER_SIZE];
	char* tmpLeft;
	char* tmpRight;
	char* packedData = NULL;
	char* subString = NULL;
	char* str = NULL;
	char** data = NULL;
	
	MPI_Comm comm;
	MPI_Status status;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	

	if(rank == master)
	{
		data = readFromFile(FILE_NAME, &K, &N, &maxIterations, &subString);
		numberOfStrings = K*K;
		if (size != numberOfStrings){
			printf("launch %d processes only\n", numberOfStrings);
			MPI_Abort(MPI_COMM_WORLD, 0);
		}
		packedData = packData(data, numberOfStrings, 2*N);
	}

	MPI_Bcast(&N, 1, MPI_INT, master, MPI_COMM_WORLD);
	MPI_Bcast(&K, 1, MPI_INT, master, MPI_COMM_WORLD);

	if(rank != master)
	{
		subString = (char*)malloc((2*N+1)*sizeof(char));
		if(subString == NULL)
			MPI_Abort(MPI_COMM_WORLD, 0);
	}
	MPI_Bcast(subString, 2*N+1, MPI_CHAR, master, MPI_COMM_WORLD);

	MPI_Bcast(&maxIterations, 1, MPI_INT, master, MPI_COMM_WORLD);

	str = (char*)malloc((2*N+1)*sizeof(char));
	if(str == NULL)
		MPI_Abort(MPI_COMM_WORLD, 0);

	MPI_Scatter(packedData, (2*N+1), MPI_PACKED, buffer, (2*N+1), MPI_PACKED, master, MPI_COMM_WORLD);

	MPI_Unpack(buffer, BUFFER_SIZE, &position, str, (2*N+1), MPI_CHAR, MPI_COMM_WORLD);


	//Creating a KxK grid
	column = K;
	row = K;
	dim[0] = column;
	dim[1] = row;
	period[0] = 1;
	period[1] = 1;
	reorder = 0;
	MPI_Cart_create(MPI_COMM_WORLD, DIM, dim, period, reorder, &comm);
	MPI_Cart_coords(comm, rank, DIM, coord);


	flagSubString flag = notFound;

	tmpLeft = (char*)malloc((N+1)*sizeof(char));
	tmpRight = (char*)malloc((N+1)*sizeof(char));
	if(tmpLeft == NULL || tmpRight == NULL)
		MPI_Abort(MPI_COMM_WORLD, 0);
	
	for(int i = 0; flag == notFound && i < maxIterations; i++)
	{
		MPI_Cart_shift(comm, 1, 1, &source, &dest);
		char* tmpOdd = substringOddEven(str, N, odd);
		MPI_Sendrecv(tmpOdd, N+1, MPI_CHAR, dest, 1, tmpLeft, N+1, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		free(tmpOdd);


		MPI_Cart_shift(comm, 0, 1, &source, &dest);
		char* tmpEven = substringOddEven(str, N, even);
		MPI_Sendrecv(tmpEven, N+1, MPI_CHAR, dest, 1, tmpRight, N+1, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		free(tmpEven);


		free(str);
		str = concat(tmpLeft, tmpRight);
		//check sub string
		if(strstr(str, subString) != NULL)
			flag = found;

		max = 0;
		MPI_Reduce(&flag, &max, 1, MPI_INT, MPI_MAX, master, MPI_COMM_WORLD);
		if(rank == master && max != 0)
			flag = found;
		// master send instruction.
		MPI_Bcast(&flag, 1, MPI_BYTE, master, MPI_COMM_WORLD);
	}
	free(tmpRight);
	free(tmpLeft);
	
	char* newData = NULL;
	
	if(flag == notFound)
	{
		if(rank == master)
			printf("The string was not found\n");
	}
	else
	{
		if(rank == master)
			newData = (char*)malloc((numberOfStrings * (N*2) + 1) * sizeof(char));
		// gather data
		MPI_Gather(str, N*2, MPI_CHAR, newData, 2*N, MPI_CHAR, master , MPI_COMM_WORLD);
		// print data
		if(rank == master)
		{
			for(int i = 0; i < numberOfStrings; i++)
			{
				printf("From process %d: ", i);
				for(int j = 0; j < 2*N; j++)
					printf("%c", newData[i*(2*N) + j]);
				printf("\n");
			}
			free(newData);
		}
	}
	
	

	if(rank == master)
	{
		for (int i = 0; i < numberOfStrings; i++)
			free(*(data + i));
		free(data);
		free(packedData);

	}
	free(subString);
	free(str);

	MPI_Finalize();
	return 0;
}

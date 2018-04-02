// Laba3_PP_C++.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <iostream>

//function sleep
#include <Windows.h>

#include "mpi.h"
#include <stdio.h>

using namespace std;

void quickSort(int* A, int N)
{
	//set pointers on original places
	int i = 0, j = N;
	int temp, p;

	//the central element
	p = A[N >> 1];

	//procedure division
	do 
	{
		while (A[i] < p)
			i++;

		while (A[j] > p)
			j--;

		if (i <= j)
		{
			temp = A[i];
			A[i] = A[j];
			A[j] = temp;
			i++; j--;
		}
	}
	while (i <= j);

	//recursive calls, if you have something to sort
	if (j > 0)
		quickSort(A, j);

	if (N > i)
		quickSort(A + i, N - i);
}

int main(int argc, char **argv)
{
	int rank, size;
	MPI_Init(&argc, &argv);
	MPI_Comm comm = MPI_COMM_WORLD;
	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &size);
	MPI_Status status;

	//is even type of iteration (0 - no, 1 - yes)
	int IsEvenIter = 1;

	//is sorted the array (0 - no, 1 - yes)
	int IsSorted = 0;

	//tags
	int sendtag = 111;
	int recvtag = 222;

	//count of values
	const int N = 16;

	//input blockly sorting array
	int A[N] = {13, 55, 59, 88, 29, 43, 71, 85, 2, 18, 40, 75, 4, 14, 22, 43};

	//blocks of data (N / p) = 4
	int piece = N / size;

	//for recieve part of block of array A (size of blocks multipy on 2 for part of blocks on other processes at swap)
	int* B = new int[piece*2];

	//0-process send pieces of blocks "A" all processes (A separate on equal pieces between all processes)
	MPI_Scatter(A, piece, MPI_INT, B, piece, MPI_INT, 0, comm);

	//for transfer values from nearest neighbor
	int* C = new int[piece * 2];

	//while the array not fully sorted
	while (IsSorted == 0)
	{
		if (IsEvenIter == 1)
		{
			//1. for even iterations
			//1.1. even rank get blocks from (rank + 1) process
			if (rank % 2 == 0)
				MPI_Sendrecv(B, piece, MPI_INT, rank + 1, sendtag, C, piece, MPI_INT, rank + 1, recvtag, comm, &status);

			//1.2. odd rank get blocks from (rank - 1) process
			if (rank % 2 == 1)
				MPI_Sendrecv(B, piece, MPI_INT, rank - 1, recvtag, C, piece, MPI_INT, rank - 1, sendtag, comm, &status);

			//add blocks from other process
			for (size_t i = piece; i < piece * 2; i++)
			{
				B[i] = C[i - piece];
			}

			//sorting
			quickSort(B, piece*2 - 1);

			//left part - 1 process, right part - 2 process
			if (rank % 2 == 0)
			{ 
				//left process
			}
			else
			{
				//right process
				for (size_t i = 0; i < piece; i++)
					B[i] = B[i + piece];
			}
			
			//next type of iteration
			IsEvenIter = 0;
		}
		else
		{
			//2. for odd iterations
			//2.1. even rank get blocks from (rank - 1) process
			if (rank % 2 == 0 && rank != 0)
				MPI_Sendrecv(B, piece, MPI_INT, rank - 1, sendtag, C, piece, MPI_INT, rank - 1, recvtag, comm, &status);

			//2.2. odd rank get blocks from (rank + 1) process
			if (rank % 2 == 1 && rank != size - 1)
				MPI_Sendrecv(B, piece, MPI_INT, rank + 1, recvtag, C, piece, MPI_INT, rank + 1, sendtag, comm, &status);

			//add blocks from other process
			for (size_t i = piece; i < piece * 2; i++)
			{
				B[i] = C[i - piece];
			}

			//sorting
			if(rank != 0 && rank != size - 1)
				quickSort(B, piece * 2 - 1);

			//left part - 1 process, right part - 2 process
			if (rank % 2 == 1)
			{
				//left process
			}
			else if(rank != 0)
			{
				//right process
				for (size_t i = 0; i < piece; i++)
					B[i] = B[i + piece];
			}

			//next type of iteration
			IsEvenIter = 1;
		}

		//gather in one place (in 0-process)
		MPI_Gather(B, piece, MPI_INT, A, piece, MPI_INT, 0, comm);

		//check all blocks (A)
		if (rank == 0)
		{
			//count of true blocks
			int counter = 0;
			for (size_t i = piece - 1; i < N - 1; i += piece)
			{
				//if end one block <= begin next block
				if (A[i] <= A[i + 1])
					counter++;
			}

			//check count of true blocks
			if(counter == size - 1)
			{
				IsSorted = 1;

				//output sorted array
				for (size_t i = 0; i < N; i++)
					cout << A[i] << " ";
				cout << endl;
			}			
		}	

		//synchronization all process
		MPI_Barrier(comm);
	}

	MPI_Finalize();
	return 0;
}
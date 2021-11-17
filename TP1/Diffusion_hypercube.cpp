#include <iostream>
#include <cstdlib>
#include <mpi.h>
#include <sstream>
#include <fstream>
#include <math.h>

int main(int nargs, char *argv[])
{
	MPI_Init(&nargs, &argv);
	int rank, nbp;

	MPI_Comm globComm;
	MPI_Comm_dup(MPI_COMM_WORLD, &globComm);

	MPI_Comm_rank(MPI_COMM_WORLD,
				  &rank);
	MPI_Comm_size(MPI_COMM_WORLD,
				  &nbp);

	int jeton = 3;
	int tag = 1234; //étiquette du message
	int msg_to_send = jeton;
	int msg_to_rcv;
	int d = 3;
	MPI_Status status;

	for (int i = 0; i < d; i++)
	{
		if (rank < pow(2, i))
		{
			MPI_Send(&msg_to_send, 1, MPI_INT, rank + pow(2, i), tag, MPI_COMM_WORLD);
			std::cout << rank << " sent " << msg_to_send << " to " << rank + pow(2, i) << "\n";
		}

		if (rank >= pow(2, i) && rank < pow(2, i + 1))
		{
			MPI_Recv(&msg_to_rcv, 1, MPI_INT, rank - pow(2, i), tag, MPI_COMM_WORLD, &status);
			std::cout << rank << " received " << msg_to_rcv << " from " << rank - pow(2, i) << "\n";
		}
	}

	MPI_Finalize();
	return EXIT_SUCCESS;
}

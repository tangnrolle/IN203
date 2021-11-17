#include <iostream>
#include <cstdlib>
#include <mpi.h>
#include <sstream>
#include <fstream>

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

    int tag = 1234; //étiquette du message
    int msg_to_send = 33;
    int msg_to_rcv;
    MPI_Status status;

    if (rank == 0)
    {
        MPI_Send(&msg_to_send, 1, MPI_INT, 1, tag, MPI_COMM_WORLD);
        MPI_Recv(&msg_to_rcv, 1, MPI_INT, nbp - 1, tag, MPI_COMM_WORLD, &status);
        std::cout << "O prints " << msg_to_rcv + 1 << "\n";
    }

    else
    {
        msg_to_send = rank * 3;
        MPI_Send(&msg_to_send, 1, MPI_INT, (rank + 1) % nbp, tag, MPI_COMM_WORLD);
        MPI_Recv(&msg_to_rcv, 1, MPI_INT, rank - 1, tag, MPI_COMM_WORLD, &status);
        std::cout << rank << " prints " << msg_to_rcv + 1 << "\n";
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}

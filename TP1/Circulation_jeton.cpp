# include <iostream>
# include <cstdlib>
# include <mpi.h>
# include <sstream>
# include <fstream>

int main( int nargs, char* argv[] )
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
    MPI_Status status;

    if (rank == 0){
        MPI_Send(&msg_to_send, 1,MPI_INT, 1, tag, MPI_COMM_WORLD);
        MPI_Recv(&msg_to_rcv,1,MPI_INT, nbp-1, tag, MPI_COMM_WORLD,&status);
        std::cout << "O received from " << nbp-1 << " " << msg_to_rcv << "\n";
    }

    else {
        MPI_Recv(&msg_to_rcv,1,MPI_INT, rank-1, tag, MPI_COMM_WORLD,&status);
        msg_to_send = msg_to_rcv + 1;
        std::cout << rank << " sent " << msg_to_send << " to " << (rank+1)%nbp << "\n";
        MPI_Send(&msg_to_send, 1,MPI_INT, (rank+1)%nbp, tag, MPI_COMM_WORLD);
        
        
    }


    MPI_Finalize();
    return EXIT_SUCCESS;
}

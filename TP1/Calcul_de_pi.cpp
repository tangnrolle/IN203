#include <chrono>
#include <random>
#include <iostream>
#include <cstdlib>
#include <mpi.h>
#include <sstream>
#include <fstream>

// Attention , ne marche qu 'en C++ 11 ou supérieur :
double approximate_pi(unsigned long nbSamples)
{
    typedef std ::chrono ::high_resolution_clock myclock;
    myclock ::time_point beginning = myclock ::now();
    myclock ::duration d = myclock ::now() - beginning;
    unsigned seed = d.count();
    std ::default_random_engine generator(seed);
    std ::uniform_real_distribution<double> distribution(-1.0, 1.0);
    double nbDarts = 0;

    // Throw nbSamples darts in the unit square [ -1:1] x [ -1:1]
    for (unsigned sample = 0; sample < nbSamples; ++sample)
    {
        double x = distribution(generator);
        double y = distribution(generator);
        // Test if the dart is in the unit disk
        if (x * x + y * y <= 1)
            nbDarts++;
    }

    return (nbDarts);
}

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
    double msg_to_send;
    double msg_to_rcv;
    MPI_Status status;

    unsigned long nbSamples = 10000000;
    unsigned long sub_nbSamples = nbSamples / nbp;
    double total_nbDarts = 0;

    typedef std ::chrono ::high_resolution_clock myclock;
    myclock ::time_point beginning = myclock ::now();

    if (rank == 0) // tâche maître
    {
        total_nbDarts += approximate_pi(sub_nbSamples);
        std::cout << "master task estimated : " << total_nbDarts << "\n";

        for (int ranki = 1; ranki < nbp; ranki++)
        {
            MPI_Recv(&msg_to_rcv, 1, MPI_DOUBLE, ranki, tag, MPI_COMM_WORLD, &status);
            total_nbDarts += msg_to_rcv;
        }

        double estimated_pi = 4 * (total_nbDarts / nbSamples);
        std::chrono::duration<double, std::milli> runtime = myclock ::now() - beginning;
        std::cout << "estimated value of pi = " << estimated_pi << " and runtime was : " << runtime.count() << "ms.\n ";
    }

    else
    {
        msg_to_send = approximate_pi(sub_nbSamples);
        MPI_Send(&msg_to_send, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD);
        std::cout << rank << " sent nbDarts " << msg_to_send << " to master task \n";
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
#include <iostream>
#include <map>
#include <vector>
#include <string>

#include <mpi.h>


int main(int argc, char * argv[])
{

    // Initialize MPI
    // Find out my identity in the default communicator
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Master
    if (rank == 0)
    {
        // ++++++++++++++++++++++++++++++
        // Master process
        // ++++++++++++++++++++++++++++++

        // check the arugmumants
        // if (argc < 2)
        // {
        //     std::cout << "Usage: " << argv[0] << std::endl;
        //     return 1;
        // }

        //wait for the workers to finish to collect the results
        //MPI_Barrier(MPI_COMM_WORLD);

    }
    else
    {
        // ++++++++++++++++++++++++++++++
        // Workers
        // ++++++++++++++++++++++++++++++

        //used for the barrier in master
        //MPI_Barrier(MPI_COMM_WORLD);
    }

    //Shut down MPI
    MPI_Finalize();

    return 0;

} // END of main


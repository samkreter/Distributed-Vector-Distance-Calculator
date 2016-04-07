#include <iostream>
#include <map>
#include <vector>
#include <string>

#include "mpi.h"
#include "include/Directory.hpp"
#include "include/parser.hpp"


int sendWork();
int doWork();
float findDist(long start1, long start2);

using MapString_t = std::map<std::string,long>;


int main(int argc, char * argv[])
{

    // Initialize MPI
    // Find out my identity in the default communicator
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    Directory dir("./");

    dir.print_dir();


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

int sendWork(){
    int threadCount;
    MPI_Comm_size(MPI_COMM_WORLD, &threadCount);

    std::map<std::string,int> bookCounterResult;


    scottgs::book_type::const_iterator verse = book.begin();

    // Start with 1, because the master is =0
    for (int rank = 1; rank < threadCount && verse!=book.end(); ++rank, ++verse)
    {
        // work tag / index
        int index = verse->first;

        const std::string line(verse->second);
        const size_t length = line.size();
        char msg[scottgs::LINE_MESSAGE_SIZE];
        line.copy(msg,length);
        msg[length] = '\0';

        MPI_Send(msg,           /* message buffer */
             scottgs::LINE_MESSAGE_SIZE,            /* buffer size */
             MPI_CHAR,          /* data item is an integer */
             rank,              /* destination process rank */
             index,         /* user chosen message tag */
             MPI_COMM_WORLD);   /* default communicator */


    }
}

int doWork(){

    shared_ptr<MapString_t> nameMap(new MapString_t);
    shared_ptr<vector<float>> dataVector(new vector<float>);
    Parser p(nameMap,dataVector);
}

float findDist(long start1, long start2){

    float sum = 0;

    //run the l1 norm formula
    for(int i = 0; i < lineLength; i++){
        sum += std::fabs(rawData[(start1+i)] - rawData[ROWMATRIXPOS(lineLength,start2,i)]);
    }

    return sum / (float) lineLength;

}

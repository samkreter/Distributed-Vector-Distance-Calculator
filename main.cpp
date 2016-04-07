#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

#include "mpi.h"
#include "include/Directory.hpp"
#include "include/parser.hpp"

#define MAX_MSG_SIZE 100
#define MAX_RESULT_SIZE 100

#define TERMINATE 0
#define FILENAME 1
#define CMPVECTOR 2
#define RESULTS 3

int sendWork(std::vector<std::string> fileNames);
int doWork();
float findDist(int lineLength,std::shared_ptr<std::vector<float>> rawData,
    int startPos,std::vector<float> cmpVec);

using MapString_t = std::map<std::string,long>;


int main(int argc, char * argv[])
{

    // Initialize MPI
    // Find out my identity in the default communicator
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    Directory dir("/cluster_dev");

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

        auto test = dir.get_files();
        test.resize(2);
        sendWork(test);



        //wait for the workers to finish to collect the results
        //MPI_Barrier(MPI_COMM_WORLD);

    }
    else
    {
        // ++++++++++++++++++++++++++++++
        // Workers
        // ++++++++++++++++++++++++++++++
        doWork();

        //used for the barrier in master
        //MPI_Barrier(MPI_COMM_WORLD);
    }

    //Shut down MPI
    MPI_Finalize();

    return 0;

} // END of main

int sendWork(std::vector<std::string> fileNames){

    int threadCount;
    MPI_Comm_size(MPI_COMM_WORLD, &threadCount);

    std::vector<std::string>::iterator currFile = fileNames.begin();

    // Start with 1, because the master is =0
    for (int rank = 1;
     rank < threadCount && currFile != fileNames.end();
     ++rank, ++currFile){
        const size_t length = (*currFile).size();
        char msg[MAX_MSG_SIZE];
        (*currFile).copy(msg,length);
        msg[length] = '\0';

        MPI_Send(msg,           /* message buffer */
             MAX_MSG_SIZE,      /* buffer size */
             MPI_CHAR,          /* data item is an integer */
             rank,              /* destination process rank */
             FILENAME,         /* user chosen message tag */
             MPI_COMM_WORLD);   /* default communicator */


    }

    for ( ;currFile != fileNames.end(); ++currFile) {

        // Receive results from a worker
        char resultMsg[MAX_RESULT_SIZE];
        MPI_Status status;

        // Receive a message from the worker
        MPI_Recv(resultMsg,             /* message buffer */
            MAX_RESULT_SIZE,   /* buffer size */
            MPI_CHAR,       /* data item is an integer */
            MPI_ANY_SOURCE,     /* Recieve from thread */
            MPI_ANY_TAG,        /* tag */
                MPI_COMM_WORLD,     /* default communicator */
                &status);

        //const int incomingIndex = status.MPI_TAG;
        const int sourceCaught = status.MPI_SOURCE;


        const size_t length = (*currFile).size();
        char msg[MAX_MSG_SIZE];
        (*currFile).copy(msg,length);
        msg[length] = '\0';

        MPI_Send(msg,       /* message buffer */
             MAX_MSG_SIZE,   /* buffer size */
             MPI_CHAR,      /* data item is an integer */
             sourceCaught,  /* destination process rank */
             FILENAME, /* user chosen message tag */
             MPI_COMM_WORLD);   /* default communicator */

    }




    for (int rank = 1; rank < threadCount; ++rank) {
        MPI_Send(0, 0, MPI_INT, rank, TERMINATE, MPI_COMM_WORLD);
    }




    return 1;
}

int doWork(){

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    char msg[MAX_MSG_SIZE];
    MPI_Status status;

    std::chrono::duration<double> read_time_elapse;


    while (1) {
        // Receive a message from the master
        MPI_Recv(msg,           /* message buffer */
            MAX_MSG_SIZE,     /* buffer size */
            MPI_CHAR,       /* data item is an integer */
            0,              /* Receive from master */
            MPI_ANY_TAG,
                MPI_COMM_WORLD,     /* default communicator */
                &status);

        // Check if we have been terminated by the master
        // exit from the worker loop
        if (status.MPI_TAG == TERMINATE) {
            std::cout<< rank << " recieved terminate signal" << std::endl;
            return 0;
        }


        std::cout<<msg<<" Rank: "<<rank<<std::endl;


        std::shared_ptr<MapString_t> nameMap(new MapString_t);
        std::map<float,std::string> results;
        std::shared_ptr<std::vector<float>> dataVector(new std::vector<float>);
        Parser p(nameMap,dataVector);

        std::vector<float> cmpVec;

        if(!p.parse_file(msg,&read_time_elapse)){
            std::cerr<<"could not parse file: "<<msg<<std::endl;
            return 0;
        }

        int lineLength = p.get_line_length();
        for(auto& file : *nameMap){
            results.insert(std::pair<float,std::string>(
                findDist(lineLength,dataVector,file.second,*dataVector),
                file.first));
        }

        for(auto& el : results){
            std::cout<<el.first<<":"<<el.second<<std::endl;
        }



        const std::string result("Im done bro");
        const size_t length = result.size();
        char resultMsg[MAX_RESULT_SIZE];
        result.copy(resultMsg,length);
        resultMsg[length] = '\0';

        MPI_Send(resultMsg,           /* message buffer */
             MAX_RESULT_SIZE,         /* buffer size */
             MPI_CHAR,              /* data item is an integer */
             0,                     /* destination process rank, the master */
             RESULTS,             /* user chosen message tag */
             MPI_COMM_WORLD);
    }







    return 0;

}

float findDist(int lineLength,std::shared_ptr<std::vector<float>> rawData,int startPos,std::vector<float> cmpVec){

    float sum = 0;
    //run the l1 norm formula
    for(int i = 0; i < lineLength; i++){
        sum += std::fabs((*rawData).at(startPos+i) - cmpVec.at(i));
    }

    return sum / (float) lineLength;

}








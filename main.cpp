#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <string.h>

#include "mpi.h"
#include "include/Directory.hpp"
#include "include/parser.hpp"

#define MAX_MSG_SIZE 100
#define MAX_RESULT_SIZE 10

//tags for OpenMPI
#define TERMINATE 0
#define FILENAME 1
#define CMPVECTOR 2
#define RESULTS 3
#define KVALUE 4

typedef struct result{
    float distance;
    char fileName[MAX_MSG_SIZE];
}result_t;


int createMPIResultStruct(MPI_Datatype* ResultMpiType);
int sendWork(std::vector<std::string> fileNames,MPI_Datatype ResultMpiType);
int doWork(MPI_Datatype ResultMpiType);
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


    MPI_Datatype ResultMpiType;
    createMPIResultStruct(&ResultMpiType);

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
        sendWork(test,ResultMpiType);



        //wait for the workers to finish to collect the results
        //MPI_Barrier(MPI_COMM_WORLD);

    }
    else
    {
        // ++++++++++++++++++++++++++++++
        // Workers
        // ++++++++++++++++++++++++++++++
        doWork(ResultMpiType);

        //used for the barrier in master
        //MPI_Barrier(MPI_COMM_WORLD);
    }

    //Shut down MPI
    MPI_Finalize();

    return 0;

} // END of main

int sendWork(std::vector<std::string> fileNames,MPI_Datatype ResultMpiType){

    int threadCount;
    MPI_Comm_size(MPI_COMM_WORLD, &threadCount);
    int fileCount = 0;


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

        fileCount++;
    }

    for ( ;currFile != fileNames.end(); ++currFile) {

        // Receive results from a worker
        result_t resultMsg[MAX_RESULT_SIZE];
        MPI_Status status;

        std::cout << "waiting to recieve" << std::endl;
        // Receive a message from the worker
        MPI_Recv(&resultMsg,             /* message buffer */
            MAX_RESULT_SIZE,   /* buffer size */
            ResultMpiType,       /* data item is an integer */
            MPI_ANY_SOURCE,     /* Recieve from thread */
            MPI_ANY_TAG,        /* tag */
                MPI_COMM_WORLD,     /* default communicator */
                &status);
        fileCount--;
        std::cout<<"recesived result: "<< resultMsg[0].distance << std::endl;

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
        fileCount++;

    }


    for (int rank = 1; rank < threadCount && fileCount != 0; ++rank) {
        // Receive results from a worker
        result_t resultMsg[MAX_RESULT_SIZE];
        MPI_Status status;

        std::cout << "waiting to recieve1" << std::endl;
        // Receive a message from the worker
        MPI_Recv(&resultMsg,             /* message buffer */
            MAX_RESULT_SIZE,   /* buffer size */
            ResultMpiType,       /* data item is an integer */
            MPI_ANY_SOURCE,     /* Recieve from thread */
            MPI_ANY_TAG,        /* tag */
                MPI_COMM_WORLD,     /* default communicator */
                &status);
        fileCount--;
        const int sourceCaught = status.MPI_SOURCE;
        std::cout<<"recesived result: "<< resultMsg[0].distance << "from: "<<sourceCaught << std::endl;
        for(int i = 0; i < 5; i++){
            std::cout<<resultMsg[i].fileName;
        }
        std::cout<<std::endl;
    }



    for (int rank = 1; rank < threadCount; ++rank) {
        MPI_Send(0, 0, MPI_INT, rank, TERMINATE, MPI_COMM_WORLD);
    }




    return 1;
}

//just a helper function for the sort with my custom structs
bool resultPairSort(const result_t& pair1, const result_t& pair2){
    return pair1.distance < pair2.distance;
}

int doWork(MPI_Datatype ResultMpiType){

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

        const int k = 100;

        // Check if we have been terminated by the master
        // exit from the worker loop
        if (status.MPI_TAG == TERMINATE) {
            std::cout<< rank << " recieved terminate signal" << std::endl;
            return 0;
        }


        std::cout<<msg<<" Rank: "<<rank<<std::endl;


        std::shared_ptr<MapString_t> nameMap(new MapString_t);
        std::vector<result_t> results;
        std::shared_ptr<std::vector<float>> dataVector(new std::vector<float>);
        Parser p(nameMap,dataVector);

        std::vector<float> cmpVec;

        if(!p.parse_file(msg,&read_time_elapse)){
            std::cerr<<"could not parse file: "<<msg<<std::endl;
            return 0;
        }
        std::cout << "finished parsing the file" << std::endl;

        int lineLength = p.get_line_length();
        int index = 0;
        for(auto& file : *nameMap){
            std::cout <<"l";
            results.push_back(result_t());
            results.at(index).distance = findDist(lineLength,
                dataVector,file.second,*dataVector);
            strcmp(results.at(index).fileName,file.first.c_str());
        }

        std::cout << "finished finding distances" << std::endl;

        std::sort(results.begin(),results.end(),resultPairSort);
        results.resize(MAX_RESULT_SIZE);

        std::cout << "sending the final results" << std::endl;


        MPI_Send(results.data(),           /* message buffer */
             MAX_RESULT_SIZE,         /* buffer size */
             ResultMpiType,              /* data item is an integer */
             0,                     /* destination process rank, the master */
             RESULTS,             /* user chosen message tag */
             MPI_COMM_WORLD);
        std::cout << "sent final results: "<<rank << std::endl;
    }







    return 0;

}

int createMPIResultStruct(MPI_Datatype* ResultMpiType){
    result_t value;
    /** Type in order in the struct */
    MPI_Datatype type[2] = { MPI_FLOAT, MPI_CHAR};
    /** Number of occurence of each type */
    int blocklen[2] = { 1, MAX_MSG_SIZE};
    /** Position offset from struct starting address */
    MPI_Aint disp[2];
    disp[0] = reinterpret_cast<const unsigned char*>(&value.distance) - reinterpret_cast<const unsigned char*>(&value);
    disp[1] = reinterpret_cast<const unsigned char*>(&value.fileName) - reinterpret_cast<const unsigned char*>(&value);

    /** Create the type */
    if(!MPI_SUCCESS == MPI_Type_create_struct(2, blocklen, disp, type, ResultMpiType)){
        throw std::runtime_error("counted create the MPI result struct");
    }
    /** Commit it*/
    if(!MPI_SUCCESS == MPI_Type_commit(ResultMpiType) ){
        throw std::runtime_error("couldn't commit the MPI results struct");
    }
}

float findDist(int lineLength,std::shared_ptr<std::vector<float>> rawData,int startPos,std::vector<float> cmpVec){

    float sum = 0;
    float* rawd = (*rawData).data();
    float* cmpRaw = cmpVec.data();
    //run the l1 norm formula
    for(int i = 0; i < lineLength; i++){
        sum += std::fabs(*(rawd+startPos+i) - *(cmpRaw+i));
    }
    return sum / (float) lineLength;

}








#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <string.h>
#include <sstream>

#include "mpi.h"
#include "include/Directory.hpp"
#include "include/parser.hpp"
#include "include/timing.hpp"

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

int output_result_vector_to_file(std::string filename, std::vector<result_t>* vec);
int output_timing_vector_to_file(std::string filename, std::vector<double> vec, int append);
bool resultPairSort(const result_t& pair1, const result_t& pair2);
int createMPIResultStruct(MPI_Datatype* ResultMpiType);
int sendWork(std::vector<std::string> fileNames,MPI_Datatype* ResultMpiType,const int k,std::vector<result_t>* finalResults);
int doWork(MPI_Datatype* ResultMpiType,const int k);
float findDist(int lineLength,std::shared_ptr<std::vector<float>> rawData,
    int startPos,std::vector<float> cmpVec);

using MapString_t = std::map<std::string,long>;


int main(int argc, char* argv[]){

    // Initialize MPI
    // Find out my identity in the default communicator
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    std::vector<result_t> finalResults;
    int k = 0;
    MPI_Datatype ResultMpiType;
    createMPIResultStruct(&ResultMpiType);

    Directory dir("/cluster_dev");

    // Master
    if (rank == 0){
        // ++++++++++++++++++++++++++++++
        // Master process
        // ++++++++++++++++++++++++++++++

        // check the arugmumants
        if (argc < 2){
            std::cout << "Usage: " << argv[0] << " [k value]" << std::endl;
            return 0;
        }


        //test for a correct k value
        try{
            k = atoi(argv[1]);
            if(k < 0){
                throw "just need to throw a random exception to catch`";
            }
        }
        catch(...){
            std::cerr<<"k must be a positive integer"<<std::endl;
            return 0;
        }

        auto test = dir.get_files();
        test.resize(2);
        sendWork(test,&ResultMpiType,k,&finalResults);



        //wait for the workers to finish to collect the results
        MPI_Barrier(MPI_COMM_WORLD);

    }
    else{
        // ++++++++++++++++++++++++++++++
        // Workers
        // ++++++++++++++++++++++++++++++
        doWork(&ResultMpiType,k);

        //used for the barrier in master
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Type_free(&ResultMpiType);

    //Shut down MPI
    MPI_Finalize();

    output_result_vector_to_file("results.csv", &finalResults);

    return 1;

} // END of main

int sendWork(std::vector<std::string> fileNames,MPI_Datatype* ResultMpiType,const int k,std::vector<result_t>* finalResults){

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

        // Receive the result array from the worker
        MPI_Recv(&resultMsg,             /* message buffer */
            MAX_RESULT_SIZE,   /* buffer size */
            *ResultMpiType,       /* data item is an integer */
            MPI_ANY_SOURCE,     /* Recieve from thread */
            MPI_ANY_TAG,        /* tag */
                MPI_COMM_WORLD,     /* default communicator */
                &status);
        fileCount--;
        std::cout<<"recesived result: "<< resultMsg[0].distance << std::endl;

        //merge results
        finalResults->insert(finalResults->end(),resultMsg,resultMsg+k);
        std::sort(finalResults->begin(),finalResults->end(),resultPairSort);
        finalResults->resize(k);

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

        // Receive the result array from the worker
        MPI_Recv(&resultMsg,
            MAX_RESULT_SIZE,
            *ResultMpiType,
            MPI_ANY_SOURCE,
            MPI_ANY_TAG,
                MPI_COMM_WORLD,
                &status);
        fileCount--;

                //merge results
        finalResults->insert(finalResults->end(),resultMsg,resultMsg+k);
        std::sort(finalResults->begin(),finalResults->end(),resultPairSort);
        finalResults->resize(k);

        const int sourceCaught = status.MPI_SOURCE;
        std::cout<<"recesived result: "<< resultMsg[0].distance << "from: "<<sourceCaught << std::endl;

    }


    //let the workers know their off shift, so relax and have a beer
    for (int rank = 1; rank < threadCount; ++rank) {
        MPI_Send(0, 0, MPI_INT, rank, TERMINATE, MPI_COMM_WORLD);
    }

    return 1;
}

//just a helper function for the sort with my custom structs
bool resultPairSort(const result_t& pair1, const result_t& pair2){
    return pair1.distance < pair2.distance;
}

int doWork(MPI_Datatype* ResultMpiType,const int k){

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
            results.push_back(result_t());
            results.at(index).distance = findDist(lineLength,
                dataVector,file.second,*dataVector);
            strcpy(results.at(index).fileName,file.first.c_str());
            index++;
        }

        std::cout << "finished finding distances" << std::endl;

        std::sort(results.begin(),results.end(),resultPairSort);


        results.resize(MAX_RESULT_SIZE);
        std::cout<<std::endl;
        std::cout << "sending the final results" << std::endl;


        MPI_Send(results.data(),           /* message buffer */
             MAX_RESULT_SIZE,         /* buffer size */
             *ResultMpiType,              /* data item is an integer */
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
    disp[0] = offsetof(result_t, distance);
    disp[1] = offsetof(result_t,fileName);
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

int output_timing_vector_to_file(std::string filename, std::vector<double> vec, int append){
    std::fstream outputFile;
    std::ostringstream ossVec;

    if(append == 0){
        outputFile.open(filename,std::fstream::out);
    }
    else{
        outputFile.open(filename, std::fstream::out | std::fstream::app);
    }

    if(outputFile.is_open()){

#ifdef POSITION
        vec.insert(vec.begin(),POSITION);
#endif

        std::copy(vec.begin(), vec.end()-1,
        std::ostream_iterator<float>(ossVec, ","));
        ossVec << vec.back();
        outputFile<<ossVec.str()<<"\n";

        outputFile.close();
        return 1;
    }
    return 0;
}

/// print the vectors to a file, for the final output
int output_result_vector_to_file(std::string filename, std::vector<result_t>* vec){
    std::cout<<"writing files"<<std::endl;
    std::ofstream outputFile(filename);
    std::ostringstream ossVec;


    if(outputFile.is_open()){

        for(auto& elem : *vec){
            ossVec<<elem.fileName<<","<<elem.distance<<std::endl;
        }

        outputFile<<ossVec.str();

        outputFile.close();
        return 1;
    }
    return 0;
}






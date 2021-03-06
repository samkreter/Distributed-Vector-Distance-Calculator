#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <string.h>
#include <sstream>
#include <fstream>

//where the real magic comes from
#include "mpi.h"

//custom header files
#include "include/Directory.hpp"
#include "include/parser.hpp"
#include "include/timing.hpp"

//message size standards
#define MAX_MSG_SIZE 100
//#define MAX_RESULT_SIZE 2000
#define SEARCH_VECTOR_SIZE 4098
#define NUM_TIME_RESULTS 3

//tags for OpenMPI
#define TERMINATE 0
#define FILENAME 1
#define CMPVECTOR 2
#define RESULTS 3
#define SEARCH_VECTOR 4
#define TIMING 5

//final results stucture
typedef struct result{
    float distance;
    char fileName[MAX_MSG_SIZE];
}result_t;

//used for the parsing convertions
using MapString_t = std::map<std::string,long>;
using searchVector_t = struct {
    std::string filename;
    std::vector<float> data;
};


int output_result_vector_to_file(std::string filename, std::vector<result_t>* vec);
int output_timing_vector_to_file(std::string filename, std::vector<double> vec, int append);
bool resultPairSort(const result_t& pair1, const result_t& pair2);
int createMPIResultStruct(MPI_Datatype* ResultMpiType);
int sendWork(std::vector<std::string> fileNames,MPI_Datatype* ResultMpiType,
    const int k,std::vector<result_t>* finalResults, std::vector<float>& searchVector,double* timeResults);
int doWork(MPI_Datatype* ResultMpiType,const int k);
float findDist(int lineLength,std::shared_ptr<std::vector<float>> rawData,
    int startPos,float* cmpData);
int getFirstVector(std::string filename, searchVector_t* returnVec);



int main(int argc, char* argv[]){

    // Initialize MPI
    // Find out my identity in the default communicator
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //used to store the final results
    std::vector<result_t> finalResults;
    //init the k value
    int k = 0;

    if(argc > 1){
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

    }

    //init the custom openmpi datatype
    MPI_Datatype ResultMpiType;
    //wrapper function to set up the custom type
    createMPIResultStruct(&ResultMpiType);




    // Master
    if (rank == 0){
        // ++++++++++++++++++++++++++++++
        // Master process
        // ++++++++++++++++++++++++++++++

        // check the arugmumants
        if (argc < 2){
            std::cout << "Usage: " << argv[0] << " [k value] [directory path:default(/cluster)]" << std::endl;
            return 0;
        }

        std::string folderPath;

        //set the default for the folder to use
        if(argc == 2){
            folderPath = "/cluster";
        }
        else{
            folderPath = argv[2];
        }


        Directory dir;

        if(!dir.set_path(folderPath)){
            std::cerr<<"Directory either not found or not a directory"<<std::endl;
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
        searchVector_t searchVector;
        Timing wallClock;
        double timeResults[NUM_TIME_RESULTS];

        //initlize timing holder
        for(int i = 0; i < NUM_TIME_RESULTS; i++){
            timeResults[i] = 0;
        }

        wallClock.start();
        //get the first vector the first file in the the directory as the search vector
        if(!getFirstVector(test.at(0),&searchVector)){
            std::cerr << "Couldn't get the search vector" << std::endl;
            return 0;
        }
        std::cout << "Sending work to workers" << std::endl;
        sendWork(test,&ResultMpiType,k,&finalResults,searchVector.data,timeResults);



        //wait for the workers to finish to collect the results
        MPI_Barrier(MPI_COMM_WORLD);

        wallClock.end();
        std::cout<<"wallClock time: "<<wallClock.get_elapse()<<std::endl;

        //get com size for timming results
        int threadCount;
        MPI_Comm_size(MPI_COMM_WORLD, &threadCount);

        //creat a vector fromt the timing results
        std::vector<double> finalTimingVec;

        finalTimingVec.push_back(threadCount);
        finalTimingVec.push_back(k);
        finalTimingVec.push_back(wallClock.get_elapse());
        finalTimingVec.insert(finalTimingVec.end(),timeResults,timeResults+NUM_TIME_RESULTS);

        //output the timing vector to the timing for all the tests
        output_timing_vector_to_file("times.csv",finalTimingVec,1);

        //output the final results to a vector
        output_result_vector_to_file("results.csv", &finalResults);
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

    return 1;

} // END of main

int getFirstVector(std::string filename,searchVector_t* returnData){
    if(!filename.empty()){
        std::ifstream csvFirstFile(filename);
        if(csvFirstFile.is_open()){

            std::string line;
            std::string entryFName;

            //read a line at a time from the file
            std::getline(csvFirstFile,line);
            std::stringstream lineStream(line);
            std::string cell;

            //get the filename for the map
            std::getline(lineStream,entryFName,',');

            //insert the file name as the key for a vector of floats into the map
            returnData->filename = entryFName;

            //use a string stream to separte the columns
            while(std::getline(lineStream,cell,',')){
                //I love try catches, that c++ life
                try{
                    returnData->data.push_back(std::stof(cell));
                }
                //not so good to just catch all but it'll have to do for now
                catch(...){
                    std::cerr<<"Could not convert string to float or rows are not the same length"<<std::endl;
                    return 0;
                }
            }
            return 1;

        }
    }
    return 0;
}


int sendWork(std::vector<std::string> fileNames,MPI_Datatype* ResultMpiType,
    const int k,std::vector<result_t>* finalResults,std::vector<float>& cmpVec,double* timeResults){
    int MAX_RESULT_SIZE = k;
    int threadCount;
    MPI_Comm_size(MPI_COMM_WORLD, &threadCount);
    int fileCount = 0;
    double times[NUM_TIME_RESULTS];


    std::vector<std::string>::iterator currFile = fileNames.begin();

    // Start with 1, because the master is =0
    for (int rank = 1;
     rank < threadCount && currFile != fileNames.end();
     ++rank, ++currFile){
        const size_t length = (*currFile).size();
        char msg[MAX_MSG_SIZE];
        (*currFile).copy(msg,length);
        msg[length] = '\0';

        MPI_Send(cmpVec.data(),
            SEARCH_VECTOR_SIZE,
            MPI_FLOAT,
            rank,
            SEARCH_VECTOR,
            MPI_COMM_WORLD);

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
        MPI_Status statusT;


        // Receive the result array from the worker
        MPI_Recv(&resultMsg,             /* message buffer */
            MAX_RESULT_SIZE,   /* buffer size */
            *ResultMpiType,       /* data item is an integer */
            MPI_ANY_SOURCE,     /* Recieve from thread */
            MPI_ANY_TAG,        /* tag */
                MPI_COMM_WORLD,     /* default communicator */
                &status);
        fileCount--;

        MPI_Recv(times,
            NUM_TIME_RESULTS,
            MPI_DOUBLE,
            MPI_ANY_SOURCE,
            TIMING,
            MPI_COMM_WORLD,
            &statusT);

        //sum up the timing results from all the workers
        for(int i = 0; i < NUM_TIME_RESULTS; i++){
            timeResults[i] += times[i];
        }

        //const int incomingIndex = status.MPI_TAG;
        const int sourceCaught = status.MPI_SOURCE;

        //merge results
        finalResults->insert(finalResults->end(),resultMsg,resultMsg+k);
        std::sort(finalResults->begin(),finalResults->end(),resultPairSort);
        finalResults->resize(k);




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
        std::cout<<"Sending "<<msg<<" to "<<sourceCaught<<std::endl;
        fileCount++;

    }


    for (int rank = 1; rank < threadCount && fileCount != 0; ++rank) {
        // Receive results from a worker
        result_t resultMsg[MAX_RESULT_SIZE];
        MPI_Status status;
        MPI_Status statusT;

        // Receive the result array from the worker
        MPI_Recv(&resultMsg,
            MAX_RESULT_SIZE,
            *ResultMpiType,
            MPI_ANY_SOURCE,
            MPI_ANY_TAG,
                MPI_COMM_WORLD,
                &status);
        fileCount--;

        MPI_Recv(times,
            NUM_TIME_RESULTS,
            MPI_DOUBLE,
            MPI_ANY_SOURCE,
            TIMING,
            MPI_COMM_WORLD,
            &statusT);

        //sum up the timing results from all the workers
        for(int i = 0; i < NUM_TIME_RESULTS; i++){
            timeResults[i] += times[i];
        }

                //merge results
        finalResults->insert(finalResults->end(),resultMsg,resultMsg+k);
        std::sort(finalResults->begin(),finalResults->end(),resultPairSort);
        finalResults->resize(k);

        const int sourceCaught = status.MPI_SOURCE;
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
    int MAX_RESULT_SIZE = k;

    char msg[MAX_MSG_SIZE];
    float cmpData[SEARCH_VECTOR_SIZE];
    double times[2];
    MPI_Status status;
    Timing parserTime;
    Timing searchTime;
    Timing workWallTime;

    workWallTime.start();
    std::chrono::duration<double> read_time_elapse;

    MPI_Recv(cmpData,
        SEARCH_VECTOR_SIZE,
        MPI_FLOAT,
        0,
        MPI_ANY_TAG,
        MPI_COMM_WORLD,
        &status);

    if(status.MPI_TAG != SEARCH_VECTOR){
        std::cout<< rank << " recieved terminate signal" << std::endl;
            return 0;
    }


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


        std::shared_ptr<MapString_t> nameMap(new MapString_t);
        std::vector<result_t> results;
        std::shared_ptr<std::vector<float>> dataVector(new std::vector<float>);
        Parser p(nameMap,dataVector);

        parserTime.start();
        if(!p.parse_file(msg,&read_time_elapse)){
            std::cerr<<"could not parse file: "<<msg<<std::endl;
            return 0;
        }
        parserTime.end();
        searchTime.start();
        int lineLength = p.get_line_length();
        int index = 0;
        for(auto& file : *nameMap){
            results.push_back(result_t());
            results.at(index).distance = findDist(lineLength,
                dataVector,file.second,cmpData);
            strcpy(results.at(index).fileName,file.first.c_str());
            index++;
        }


        std::sort(results.begin(),results.end(),resultPairSort);


        results.resize(MAX_RESULT_SIZE);

        searchTime.end();

        MPI_Send(results.data(),           /* message buffer */
             MAX_RESULT_SIZE,         /* buffer size */
             *ResultMpiType,              /* data item is an integer */
             0,                     /* destination process rank, the master */
             RESULTS,             /* user chosen message tag */
             MPI_COMM_WORLD);

        workWallTime.end();

        //add the times to an array to be sent to the master
        times[0] = parserTime.get_elapse();
        times[1] = searchTime.get_elapse();
        times[2] = workWallTime.get_elapse();

        //send out the timing results to be compiled
        MPI_Send(times,
            NUM_TIME_RESULTS,
            MPI_DOUBLE,
            0,
            TIMING,
            MPI_COMM_WORLD);

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

float findDist(int lineLength,std::shared_ptr<std::vector<float>> rawData,int startPos,float* cmpRaw){
    float sum = 0;
    float* rawd = (*rawData).data();
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






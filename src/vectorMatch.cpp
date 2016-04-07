#include "../include/vectorMatch.hpp"


//simle converstion for the array indxing to real index
# define ROWMATRIXPOS(rowSize , row, col) (rowSize * row) + col



VectorMatch::VectorMatch(std::shared_ptr<MapString_t> nameMap, float* rawData, long lineLength){
    this->nameMap = nameMap;
    this->rawData = rawData;
    this->lineLength = lineLength;

    //create the line number to name map
    for(auto& pair : *(this->nameMap)){
        (*lineNumMap).insert(std::pair<long,std::string>(pair.second,pair.first));
    }

}

float VectorMatch::findDist(long start1, long start2){

    float sum = 0;

    //run the l1 norm formula
    for(int i = 0; i < lineLength; i++){
        sum += std::fabs(rawData[(start1+i)] - rawData[ROWMATRIXPOS(lineLength,start2,i)]);
    }

    return sum / (float) lineLength;

}

/// print the vectors to a file, for the final output
int output_vector_to_file(std::string filename, std::vector<VectorMatch::nameKeyPair>* vec){

    std::ofstream outputFile(filename);
    std::ostringstream ossVec;


    if(outputFile.is_open()){

        for(auto& elem : *vec){
            ossVec<<elem.filename<<","<<elem.dist<<std::endl;
        }

        outputFile<<ossVec.str();

        outputFile.close();
        return 1;
    }
    return 0;
}


//just a helper function for the sort with my custom structs
bool shmKeyPairSort(const VectorMatch::storeKeyPair& pair1, const VectorMatch::storeKeyPair& pair2){
    return pair1.dist < pair2.dist;
}



int VectorMatch::computVectorMatch(std::string cmpFile, int k, int p,std::chrono::duration<double>* time_elapse){


    //start and end for times the proc work
    std::chrono::time_point<std::chrono::system_clock> start, end;


    long cmpVecPos = 0;

    try{
        cmpVecPos = nameMap->at(cmpFile);
    }
    catch(...){
        std::cerr<<"File not found in the database for comapring\n";
        return 0;
    }



    //get the number of lines each proc gets to process
    int divNum = (int) (nameMap->size() / p);


    //create the main store for the threads to put their results
    storeKeyPair* mainStore = new storeKeyPair[p * k ];




    start = std::chrono::system_clock::now();
    std::thread* t = new std::thread[p];

    //this loops through and forks enough procs to process each file
    // 1 proc per file
    for(int i = 0; i < p; i++){
        t[i] = std::thread(&VectorMatch::threadWork,this,k, p, cmpVecPos, divNum, i, mainStore);

    }

    //wait for all threads to finish
    for(int i = 0; i < p; i++){
        t[i].join();
    }


    //store the final results with line numbers
    std::vector<storeKeyPair> finalResultsNum(mainStore,mainStore+(k*p));

    //hold the final filname matches
    std::vector<nameKeyPair> finalResultsName(k);

    //sort the final results
    std::sort(finalResultsNum.begin(),finalResultsNum.end(),shmKeyPairSort);

    //cut them off at the knees
    finalResultsNum.resize(k);




    int indexer = 0;
    for(auto elem : finalResultsNum){
        // std::cout<<elem.lineNum<<" ";
        finalResultsName.push_back(nameKeyPair());
        finalResultsName.at(indexer).filename = (*lineNumMap).at(elem.lineNum);
        finalResultsName.at(indexer).dist = elem.dist;
        indexer++;
    }


    //final cut off at the knees
    finalResultsName.resize(k);

    end = std::chrono::system_clock::now();

    //get the output out to that csv, yea


    output_vector_to_file("results.csv",&finalResultsName);

    *time_elapse = end - start;

    std::cout<<"Time for parent processing: "<<(*time_elapse).count()<<std::endl;



    return 1;

}

void VectorMatch::threadWork(int k, int p, long cmpVecPos, int divNum, int i,storeKeyPair* mainStore) {
    long lineStatus = 0;
    int procNum = i;

    //store the results of the vector
    std::map<float,int> results;


    int topBound = ((procNum * divNum) + divNum);
    if(i == (p-1)){
        topBound += this->nameMap->size() % p;
    }


    //get the distances and store them into a map that auto sorts by distance
    for(long j = procNum * divNum; j < topBound -1; j++){
        //add the distance to the results map
        results.insert(std::pair<float,int>(findDist(cmpVecPos, j), j * this->lineLength));

    }



    //add the results to the proper segment of the shared memory
    int count = procNum * k;
    int kcount = 0;

    for(auto& pair : results){
        if(kcount >= k){
            break;
        }
        mainStore[count].dist = pair.first;
        mainStore[count].lineNum = pair.second;
        count++;
        kcount++;
    }


    //not usre if this is going to work but lets try
    //zero out the contents
    for(;count <= ((procNum*k)+k); count++){
        mainStore[count].dist = FLT_MAX;
    }


}







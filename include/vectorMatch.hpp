#ifndef VECTORMATCH_HPP__
#define VECTORMATCH_HPP__

#include <vector>
#include <iostream>
#include <memory>
#include <cmath>
#include <memory>
#include <map>
#include <iterator>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cfloat>
#include <exception>
#include <set>
#include <thread>
#include <omp.h>



class VectorMatch{



public:
    //I really don't want to write this 1000 times, so thank you c++ typedef
    using MapString_t = std::map<std::string,long>;
    using InvMapString_t = std::map<long,std::string>;
    using DataVector_t = std::vector<float>;

    //store the pairs in shared memory
    using storeKeyPair = struct{
        float dist;
        long lineNum;
    };

    //used to store the converted key pairs
    using nameKeyPair = struct{
        std::string filename;
        float dist;
    };

    /// assing the data pointer to the local struct
    /// \param data: pointer to the data map with the data
    /// \return nothing, it just a poor little constructor
    VectorMatch(std::shared_ptr<MapString_t> nameMap, float* rawData, long lineLength);

    /// Main function that executes the vector comparing
    /// \param cmpFIle: filename that you want to compare to the others
    /// \param k: top k matches to return
    /// \param p: number of procs to use in this case
    /// \param time_elapse: pointer to where to put the total duration of the processing
    /// \return int error code
    int computVectorMatch(std::string cmpFile, int k, int p,std::chrono::duration<double>* time_elapse);
private:
    /// pointer to the datamap
    std::shared_ptr<MapString_t> nameMap;
    std::shared_ptr<InvMapString_t> lineNumMap = std::shared_ptr<InvMapString_t>(new InvMapString_t);
    float* rawData;
    long lineLength = 0;

    /// finds the distance with two vectors with their startin gpoints
    float findDist(long start1, long start2);

    void threadWork(int k, int p, long cmpVecPos, int divNum, int i,storeKeyPair* mainStore);

};



#endif
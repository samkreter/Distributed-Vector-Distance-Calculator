#include "../include/parser.hpp"



int Parser::parse_file(std::string filename, std::chrono::duration<double>* time_elapse){

    if(!filename.empty()){

        long lineSize = 0;
        long indexer = 0;
        //set up the chono vars
        std::chrono::time_point<std::chrono::system_clock> start, end;
        //open the input file
        std::ifstream csvFile(filename);

        //make sure all good in the file
        if(csvFile.is_open()){

            std::string line;
            std::string entryFName;
            //get the start time to find time elapse
            start = std::chrono::system_clock::now();
            //read a line at a time from the file
            while(std::getline(csvFile,line)){
                std::stringstream lineStream(line);
                std::string cell;

                //get the filename for the map
                std::getline(lineStream,entryFName,',');

                //insert the file name as the key for a vector of floats into the map
                nameMap->insert(std::pair<std::string,long>(entryFName,indexer));

                lineSize = 0;
                //use a string stream to separte the columns
                while(std::getline(lineStream,cell,',')){
                    lineSize++;
                    //I love try catches, that c++ life
                    try{
                        dataVector->push_back(std::stof(cell));
                        indexer++;
                    }
                    //not so good to just catch all but it'll have to do for now
                    catch(...){
                        std::cerr<<"Could not convert string to float or rows are not the same length"<<std::endl;
                        return 0;
                    }
                }
            }
            //get the end time
            end = std::chrono::system_clock::now();

            this->lineLength = lineSize;
            //do math good
            *time_elapse = end - start;

            //always close that file
            csvFile.close();
            return 1;
        }
        std::cerr<<"Couldn't open file\n";
        return 0;
    }
    std::cerr<<"Filename Can't be empty\n";
    return 0;

}


//there is a map entry for each line so let the map struct do the actual work.
// its alot more efficent, smart people wrote that, not just some random kid in comp sci
// college. Probably some nice person in a basement that is covered with arduinos and doritos.
size_t Parser::num_of_lines(){
    return nameMap->size();
}



long Parser::get_line_length(){
    return this->lineLength;
}

int Parser::output_vector_to_file(std::string filename, std::vector<float> vec, std::vector<float> vec2){
    std::ofstream outputFile(filename);
    std::ostringstream ossVec, ossVec2;

    if(outputFile.is_open()){

        outputFile<<"Mins,";
        std::copy(vec.begin(), vec.end()-1,
        std::ostream_iterator<float>(ossVec, ","));
        ossVec << vec.back();
        outputFile<<ossVec.str()<<"\n";

        outputFile<<"Maxs,";
        std::copy(vec2.begin(), vec2.end()-1,
        std::ostream_iterator<float>(ossVec2, ","));
        ossVec2 << vec2.back();
        outputFile<<ossVec2.str()<<"\n";



        outputFile.close();
        return 1;
    }
    return 0;
}


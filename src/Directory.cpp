#include "../include/Directory.hpp"

int Directory::set_path(std::string dirPath){
    if(this->_getFiles(dirPath)){
        return 1;
    }
    return 0;
}


void Directory::print_dir(){
    for(auto& file : this->_dirContents){
        std::cout<<file.first<<std::endl;
        for(auto& name : file.second){
            std::cout<<name<<std::endl;
        }
        std::cout<<std::endl;
    }
}

std::vector<std::string> Directory::get_files(){
    std::vector<std::string> names;
    for(auto& file : this->_dirContents){
        if(file.first == std::string("REGULAR")){
            for(auto& name : file.second){
                names.push_back(name.string());
            }
        }
    }

    return names;
}



int Directory::_getFiles(std::string dir){
    // Define my map keys
    const std::string regular_file("REGULAR");
    const std::string directory_file("DIRECTORY");
    const std::string other_file("OTHER");

    // This is a return object
    // REGULAR -> {file1r,file2r,...,fileNr}
    // DIRECTORY -> {file1d,file2d,...,fileNd}
    // ...


    // Change to the absolute system path, instead of relative
    //boost::filesystem::path dirPath(boost::filesystem::system_complete(dir));
    boost::filesystem::path dirPath(dir);

    // Verify existence and directory status
    if ( !boost::filesystem::exists( dirPath ) ){
        std::stringstream msg;
        msg << "Error: " << dirPath.string() << " does not exist " << std::endl;
        return 0;
    }

    if ( !boost::filesystem::is_directory( dirPath ) ){
        std::stringstream msg;
        msg << "Error: " << dirPath.string() << " is not a directory " << std::endl;
        return 0;
    }

#ifdef DEBUG
    std::cout << "Processing directory: " << dirPath.directory_string() << std::endl;
#endif
    // A director iterator... is just that,
    // an iterator through a directory... crazy!
    boost::filesystem::directory_iterator end_iter;
    for ( boost::filesystem::directory_iterator dir_itr( dirPath );
        dir_itr != end_iter;
        ++dir_itr ){
        // Attempt to test file type and push into correct list
        try{
            if ( boost::filesystem::is_directory( dir_itr->status() ) ){
                // Note, for path the "/" operator is overloaded to append to the path
                _dirContents[directory_file].push_back(dir_itr->path());
#ifdef DEBUG
                std::cout << dir_itr->path().filename() << " [directory]" << std::endl;
#endif
            }
            else if ( boost::filesystem::is_regular_file( dir_itr->status() ) ){
                _dirContents[regular_file].push_back(dir_itr->path());
#ifdef DEBUG
                std::cout << "Found regular file: " << dir_itr->path().filename() << std::endl;
#endif
            }
            else{
                _dirContents[other_file].push_back(dir_itr->path());
#ifdef DEBUG
                std::cout << dir_itr->path().filename() << " [other]" << std::endl;
#endif
            }

        }
        catch ( const std::exception & ex ){
            std::cerr << dir_itr->path().filename() << " " << ex.what() << std::endl;
            return 0;
        }
    }

    return 1;
}


Directory::Directory(std::string dirPath):{
    if(!this->_getFiles(dirPath)){
        std::err<<"File path doens't exist"<<std::endl;
    }
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
    std::map<std::string,path_list_type> directoryContents;

    // Change to the absolute system path, instead of relative
    //boost::filesystem::path dirPath(boost::filesystem::system_complete(dir));
    boost::filesystem::path dirPath(dir);

    // Verify existence and directory status
    if ( !boost::filesystem::exists( dirPath ) ){
        std::stringstream msg;
        msg << "Error: " << dirPath.file_string() << " does not exist " << std::endl;
        throw std::runtime_error(msg.str());
    }

    if ( !boost::filesystem::is_directory( dirPath ) ){
        std::stringstream msg;
        msg << "Error: " << dirPath.file_string() << " is not a directory " << std::endl;
        throw std::runtime_error(msg.str());
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
                directoryContents[directory_file].push_back(dir_itr->path());
#ifdef DEBUG
                std::cout << dir_itr->path().filename() << " [directory]" << std::endl;
#endif
            }
            else if ( boost::filesystem::is_regular_file( dir_itr->status() ) ){
                directoryContents[regular_file].push_back(dir_itr->path());
#ifdef DEBUG
                std::cout << "Found regular file: " << dir_itr->path().filename() << std::endl;
#endif
            }
            else{
                directoryContents[other_file].push_back(dir_itr->path());
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

    this->_dirContents = directoryContents;
    return 1;
}

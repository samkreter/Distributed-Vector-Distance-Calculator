#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <map>
#include <sstream>

// See the boost documentation for the filesystem
// Especially: http://www.boost.org/doc/libs/1_41_0/libs/filesystem/doc/reference.html#Path-decomposition-table
// Link against boost_filesystem-mt (for multithreaded) or boost_filesystem
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

class Directory{
public:
    typedef std::list<boost::filesystem::path> path_list_type;

    Directory(std::string dirPath);
    void print_dir();

private:
    std::map<std::string,path_list_type> _dirContents;
    int _getFiles(std::string);

};
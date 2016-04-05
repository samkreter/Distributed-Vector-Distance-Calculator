#include <iostream>
#include <string>
#include <boost/filesystem.hpp>

// See the boost documentation for the filesystem
// Especially: http://www.boost.org/doc/libs/1_41_0/libs/filesystem/doc/reference.html#Path-decomposition-table
// Link against boost_filesystem-mt (for multithreaded) or boost_filesystem
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

class Directory
{
public:
    Directory(std::string dirPath);
    ~Directory();
private:
    std::string _dirPath;

};
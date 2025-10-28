#include "File.h"

std::string File::Read(std::string path)
{
    std::string line, res;
    std::ifstream in;
    in.open(path);
    if (in.is_open()) {
        while (std::getline(in, line))
        {
            res += line + "\n";
        }
    }

    return res;
}

#include "Util.h"
#include <ctime>
#include <iostream>
#include <locale>

namespace Util {

std::string CurrentDateTime(const char* fmt)
{
    std::time_t t = std::time(nullptr);
    char mbstr[100] = "";
    std::strftime(mbstr, sizeof(mbstr), fmt, std::localtime(&t));
    return std::string(mbstr);
}

std::vector<std::string> Split(const std::string & s, const char* sep)
{
    std::vector<std::string>  res;
    std::string delim(sep);

    auto start = 0U;
    auto end = s.find(delim);
    while (end != std::string::npos)
    {
        res.push_back(s.substr(start, end - start));
        start = end + delim.length();
        end = s.find(delim, start);
    }
    res.push_back(s.substr(start, end));
    return res;
}

#if defined(_WIN32)
#include <io.h>
#include <direct.h>

void Mkdir(const std::string & dir)
{
    _mkdir(dir.c_str());
}

std::string DIR_SEPARATOR = "\\";

#elif defined(__unix__)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

void Mkdir(const std::string & dir)
{
    mkdir(dir.c_str(), 0777);
}

std::string DIR_SEPARATOR = "/";

#else

#error Unsupported platform!

#endif

}

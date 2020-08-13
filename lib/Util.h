#pragma once

#include <string>
#include <vector>

namespace Util {

std::string CurrentDateTime(const char* fmt);
std::vector<std::string> Split(const std::string & s, const char* sep);
void Mkdir(const std::string & dir);
extern std::string DIR_SEPARATOR;

}

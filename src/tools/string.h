#pragma once

#include <string>

namespace util
{
    std::string getFileExtension(const std::string& filename);
    bool stricmp(const std::string& str1, const std::string& str2);
}

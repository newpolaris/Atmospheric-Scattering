#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <regex>

namespace util
{
    std::string getFileExtension(const std::string& filename);
    bool stricmp(const std::string& str1, const std::string& str2);

    /**
       Helper code to unpack variadic arguments
    */
    namespace internal
    {
        template<typename T>
        void unpack(std::vector<std::string> &vbuf, T t)
        {
            std::stringstream buf;
            buf << t;
            vbuf.push_back(buf.str());
        }
        template<typename T, typename ...Args>
        void unpack(std::vector<std::string> &vbuf, T t, Args &&... args)
        {
            std::stringstream buf;
            buf << t;
            vbuf.push_back(buf.str());
            unpack(vbuf, std::forward<Args>(args)...);
        }
    }

    // https://stackoverflow.com/questions/23412978/c11-equivalent-to-boost-format
    /**
        Python-like string formatting
     */
    template<typename ... Args>
    std::string format(const std::string& fmt, Args ... args)
    {
        std::vector<std::string> vbuf;  // store arguments as strings
        std::string in(fmt), out;    // unformatted and formatted strings
        std::regex re_arg("\\{\\b\\d+\\b\\}");  // search for {0}, {1}, ...
        std::regex re_idx("\\b\\d+\\b");        // search for 0, 1, ...
        std::smatch m_arg, m_idx;               // store matches
        size_t idx = 0;                         // index of argument inside {...}

        // Unpack arguments and store them in vbuf
        internal::unpack(vbuf, std::forward<Args>(args)...);

        // Replace all {x} with vbuf[x]
        while (std::regex_search(in, m_arg, re_arg)) {
            out += m_arg.prefix();
            auto argstr = m_arg[0].str();
            if (std::regex_search(argstr, m_idx, re_idx)) {
                idx = std::stoi(m_idx[0].str());
            }
            if(idx < vbuf.size()) {
                out += std::regex_replace(argstr, re_arg, vbuf[idx]);
            }
            in = m_arg.suffix();
        }
        out += in;
        return out;
    }
}

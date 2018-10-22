/*-----------------------------------------------------------------------
  Copyright (c) 2014, NVIDIA. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Neither the name of its contributors may be used to endorse 
     or promote products derived from this software without specific
     prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/
/*
 * This file contains code derived from glf by Christophe Riccio, www.g-truc.net
 */

#define NV_LINE_MARKERS 1

#include <GL/glew.h>
#include <GLType/ProgramManager.h>
#include <tools/misc.hpp>
#include <cstdarg>

namespace nv_helpers_gl
{
    std::string parseInclude(std::string const & Line, std::size_t const & Offset)
    {
        std::string Result;

        std::string::size_type IncludeFirstQuote = Line.find("\"", Offset);
        std::string::size_type IncludeSecondQuote = Line.find("\"", IncludeFirstQuote + 1);

        return Line.substr(IncludeFirstQuote + 1, IncludeSecondQuote - IncludeFirstQuote - 1);
    }

    std::string format(const char* msg, ...)
    {
        std::size_t const STRING_BUFFER(8192);
        char text[STRING_BUFFER];
        va_list list;

        if(msg == 0)
            return std::string();

        va_start(list, msg);
        vsprintf(text, msg, list);
        va_end(list);

        return std::string(text);
    }


    inline std::string fixFilename(std::string const & filename)
    {
    #ifdef _WIN32
        // workaround for older versions of nsight
        std::string fixedname;
        for(size_t i = 0; i < filename.size(); i++){
            char c = filename[i];
            if(c == '/' || c == '\\'){
                fixedname.append("\\\\");
            }
            else{
                fixedname.append(1, c);
            }
        }
        return fixedname;
    #else
        return filename;
    #endif
    }

    inline std::string markerString(int line, std::string const & filename, int fileid)
    {
        if(GLEW_ARB_shading_language_include){
	#if __APPLE__
            return format("//#line %d \"", line) + fixFilename(filename) + std::string("\"\n");
	#else
            return format("#line %d \"", line) + fixFilename(filename) + std::string("\"\n");
	#endif
        }
        else{
	#if __APPLE__
            return format("//#line %d %d\n", line, fileid);
	#else
            return format("#line %d %d\n", line, fileid);
	#endif
        }
    }

    std::string getContent(std::string const & name, const std::vector<std::string>& directories, 
        const IncludeRegistry &includes,
        std::string & filename)
    {
        // check registered includes first
        for(std::size_t i = 0; i < includes.size(); ++i)
        {
            if(includes[i].name != name) continue;

            filename = nv_helpers::findFile(includes[i].filename, directories);
            std::string content = nv_helpers::loadFile(filename, includes[i].content.empty());
            if(content.empty()) return includes[i].content;
            return content;
        }

        // fall back
        filename = nv_helpers::findFile(name, directories);
        return nv_helpers::loadFile(filename);
    }

    std::string manualInclude (
        std::string const & filenameorig,
        std::string const & source,
        std::string const & prepend,
        const std::vector<std::string>& directories,
        const IncludeRegistry &includes)
    {
        std::string filename = filenameorig;

        if (source.empty()){
            return std::string();
        }

        std::stringstream stream;
        stream << source;
        std::string line, text;

        // Handle command line defines
        text += prepend;
    #if NV_LINE_MARKERS
        text += markerString(1, filename, 0);
    #endif
        int lineCount = 0;
        while(std::getline(stream, line))
        {
            std::size_t Offset = 0;
            lineCount++;

            // Version
            Offset = line.find("#version");
            if(Offset != std::string::npos)
            {
                std::size_t CommentOffset = line.find("//");
                if(CommentOffset != std::string::npos && CommentOffset < Offset)
                    continue;

                // Reorder so that the #version line is always the first of a shader text
                text = line + std::string("\n") + text + std::string("//") + line + std::string("\n");
                continue;
            }

            // Include
            Offset = line.find("#include");
            if(Offset != std::string::npos)
            {
                std::size_t CommentOffset = line.find("//");
                if(CommentOffset != std::string::npos && CommentOffset < Offset)
                    continue;

                std::string Include = parseInclude(line, Offset);

                {
                    size_t it = filename.find_last_of("/");
                    std::string path = filename.substr(0, it);
                    std::vector<std::string> dirs;
                    for (auto it : directories)
                        dirs.push_back(it + "/" + path);
                    dirs.insert(dirs.end(), directories.begin(), directories.end());
                    std::string PathName;
                    std::string Source = getContent(Include, dirs, includes, PathName);

                    assert(!Source.empty());

                    if(!Source.empty())
                    {
                    #if NV_LINE_MARKERS
                        text += markerString(1, PathName, 1);
                    #endif
                        text += manualInclude(Include, Source, "", dirs, includes);
                    #if NV_LINE_MARKERS
                        text += std::string("\n") + markerString(lineCount + 1, filename, 0);
                    #endif
                    }
                }

                continue;
            }

            text += line + "\n";
        }

        return text;
    }
}

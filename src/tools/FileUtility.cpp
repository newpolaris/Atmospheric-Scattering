#include <fstream>
#include <tools/FileUtility.h>

using namespace util;

namespace util
{
    BytesArray NullFile = std::make_shared<FileContainer>(FileContainer());

    BytesArray ReadFileSync(const std::string& fileName)
    {
        std::ifstream inputFile;
        inputFile.open(fileName, std::ios::binary | std::ios::ate);
        if(!inputFile.is_open())
            return NullFile;
        auto filesize = static_cast<size_t>(inputFile.tellg());
        BytesArray buf = std::make_shared<FileContainer>(filesize * sizeof(char));
        inputFile.ignore(std::numeric_limits<std::streamsize>::max());
        inputFile.seekg(std::ios::beg);
        inputFile.read(reinterpret_cast<char*>(buf->data()), filesize);
        inputFile.close();
        return buf;
    }

    uint32_t ReadUint(bufferstream& is)
    {
        uint32_t t;
        Read(is, t);
        return t;
    }

    uint16_t ReadShort(bufferstream& is)
    {
        uint16_t t;
        Read(is, t);
        return t;
    }
}

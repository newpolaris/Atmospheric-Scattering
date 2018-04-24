#include <fstream>
#include <tools/FileUtility.h>
#include <zlib.h>
#include <algorithm>

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

    bool WriteFileSync(const std::string& fileName, const BytesArray& plainSource)
    {
        std::ofstream outputFile;
        outputFile.open(fileName, std::ios::binary);
        if (!outputFile.is_open())
            return false;
        auto data = reinterpret_cast<char*>(plainSource->data());
        auto size = plainSource->size();
        outputFile.write(data, size);
        outputFile.close();
        return true;
    }

    BytesArray inflate(const BytesArray& compressedSource, int32_t& errnum, std::uint32_t chunkSize = 0x100000)
    {
        // Create a dynamic buffer to hold compressed blocks
        std::vector<std::unique_ptr<Bytef>> blocks;

        z_stream stream = { 0, };
        stream.data_type = Z_BINARY;
        stream.total_in = compressedSource->size();
        stream.avail_in = compressedSource->size();
        stream.next_in = (Bytef*)compressedSource->data();

        // 15 window bits, and the +32 tells zlib to to detect if using gzip or zlib
        errnum = inflateInit2(&stream, (15 + 32));

        while (errnum == Z_OK || errnum == Z_BUF_ERROR)
        {
            stream.avail_out = chunkSize;
            stream.next_out = new Bytef[chunkSize];
            blocks.emplace_back(stream.next_out);
            errnum = inflate(&stream, Z_NO_FLUSH);
        }

        if (errnum != Z_STREAM_END)
        {
            inflateEnd(&stream);
            return NullFile;
        }

        assert(stream.total_out > 0);

        BytesArray bytesArray = std::make_shared<FileContainer>(stream.total_out);

        void* curDest = bytesArray->data();
        size_t remaining = bytesArray->size();

        for (size_t i = 0; i < blocks.size(); i++)
        {
            assert(remaining > 0);
            size_t copySize = std::min(remaining, (size_t)chunkSize);
            memcpy(curDest, blocks[i].get(), copySize);
            curDest = (uint8_t*)curDest + copySize;  
            remaining -= copySize;
        }

        inflateEnd(&stream);

        return bytesArray;
    }

    BytesArray deflate(const BytesArray& planeSource, int32_t& errnum, std::uint32_t chunkSize = 0x100000)
    {
        std::vector<std::unique_ptr<Bytef>> blocks;
        Bytef* curPos = reinterpret_cast<Bytef*>(planeSource->data());
        z_stream stream = { 0, };
        stream.avail_in = planeSource->size();
        stream.next_in = curPos;

        errnum = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
        while (errnum == Z_OK || errnum == Z_BUF_ERROR)
        {
            stream.avail_out = chunkSize;
            stream.next_out = new Bytef[chunkSize];
            blocks.emplace_back(stream.next_out);
            errnum = deflate(&stream, Z_FINISH);
        }

        if (errnum != Z_STREAM_END)
        {
            deflateEnd(&stream);
            return NullFile;
        }

        assert(stream.total_out > 0);

        BytesArray bytesArray = std::make_shared<FileContainer>(stream.total_out);

        void* curDest = bytesArray->data();
        size_t remaining = bytesArray->size();

        for (size_t i = 0; i < blocks.size(); i++)
        {
            assert(remaining > 0);
            size_t copySize = std::min(remaining, (size_t)chunkSize);
            memcpy(curDest, blocks[i].get(), copySize);
            curDest = (uint8_t*)curDest + copySize;  
            remaining -= copySize;
        }

        deflateEnd(&stream);

        return bytesArray;
    }

    BytesArray DecompressFile(const std::string& fileName)
    {
        BytesArray compressed = ReadFileSync(fileName);
        if (compressed == NullFile)
            return NullFile;

        int32_t errorno = 0;
        BytesArray decompressed = inflate(compressed, errorno);
        if (decompressed->size() == 0)
        {
            printf("Failed to decompress file %s\n", fileName.c_str());
            return NullFile;
        }
        return decompressed;
    }

    bool CompressFile(const std::string& fileName, const BytesArray& plainSource)
    {
        int32_t errorno = 0;
        BytesArray compressed = deflate(plainSource, errorno);
        if (compressed->size() == 0)
        {
            printf("Failed to decompress file %s\n", fileName.c_str());
            return false;
        }
        return WriteFileSync(fileName, compressed);
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

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <istream>
#include <iostream>
#include <sstream>
#include <streambuf>

namespace util
{
    // istream compatible
    using StorageType = std::istream::char_type;
    using bufferstream = std::basic_istream<StorageType, std::char_traits<StorageType>>;
    using FileContainer = std::vector<StorageType>;
    using BytesArray = std::shared_ptr<FileContainer>;

    template<typename CharT, typename TraitsT = std::char_traits<CharT> >
    class Vectorwrapbuf : public std::basic_streambuf<CharT, TraitsT> {
    public:
        Vectorwrapbuf(BytesArray vec): m_Vec(vec)
        {
			this->setg(m_Vec->data(), m_Vec->data(), m_Vec->data() + m_Vec->size());
        }
        BytesArray m_Vec;
    };

    template<typename CharT, typename TraitsT = std::char_traits<CharT> >
    class Vectorstream : public std::basic_istream<CharT, TraitsT> {
    public:
        Vectorstream(BytesArray vec): std::istream(&m_buf), m_buf(vec)
        {
        }
        Vectorwrapbuf<CharT, TraitsT> m_buf;
    };
    using ByteArrayWrapBuf = Vectorwrapbuf<StorageType>;
    using ByteStream = Vectorstream<StorageType>;

    extern BytesArray NullFile;

    // Reads the entire contents of a binary file.  
    BytesArray ReadFileSync(const std::string& fileName, std::ios_base::openmode mode = 0);
    bool WriteFileSync(const std::string& fileName, const BytesArray& plainSource);

    BytesArray DecompressFile(const std::string& fileName);
    bool CompressFile(const std::string& fileName, const BytesArray& plainSource);

    template <typename T, typename R>
    void Read(std::basic_istream<T, std::char_traits<T>>& is, R& t, uint32_t size)
    {
        static_assert(size <= sizeof(R), "buffer overflow");
        is.read(reinterpret_cast<T*>(&t), size);
    }

    template <typename T, typename R>
    void Read(std::basic_istream<T, std::char_traits<T>>& is, R& t)
    {
        is.read(reinterpret_cast<T*>(&t), sizeof(R));
    }

#if USE_GLM_READ_BYTE 
    template <typename T>
    void ReadPosition(basic_istream<T, char_traits<T>>& is, glm::vec3& t, bool bRH)
    {
        Read(is, t);
        if(bRH) t.z *= -1.0;
    }

    template <typename T>
    void ReadNormal(basic_istream<T, char_traits<T>>& is, glm::vec3& t, bool bRH)
    {
        Read(is, t);
        if(bRH) t.z *= -1.0;
    }

    template <typename T>
    void ReadRotation(basic_istream<T, char_traits<T>>& is, glm::vec3& t, bool bRH)
    {
        Read(is, t);
        if(bRH) t.x *= -1.0;
        if(bRH) t.y *= -1.0;
    }

    // Quaternion
    template <typename T>
    void ReadRotation(basic_istream<T, char_traits<T>>& is, glm::vec4& t, bool bRH)
    {
        Read(is, t);
        if(bRH) t.x *= -1.0;
        if(bRH) t.y *= -1.0;
    }
#endif

    template <typename T, typename R>
    void Read(std::basic_istream<T, std::char_traits<T>>& is, std::vector<R>& t)
    {
        auto data = reinterpret_cast<T*>(t.data());
        is.read(data, sizeof(T) * (t.size() * sizeof(R)));
    }

    uint32_t ReadUint(bufferstream& is);
    uint16_t ReadShort(bufferstream & is);

    template <typename T>
    T Read(bufferstream& is)
    {
        T t;
        Read(is, t);
        return t;
    }

    template <typename T, typename R>
    void Write(std::basic_ostream<T, std::char_traits<T>>& is, const R& t)
    {
        is.write(reinterpret_cast<const T*>(&t), sizeof(R));
    }

} // namespace Utility

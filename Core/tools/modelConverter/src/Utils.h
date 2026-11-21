#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace RB::ModelConverter
{
    // Copies into a fixed size, null terminated char array. Truncates if needed
    inline void CopyName(char* dst, size_t capacity, const std::string& src)
    {
        std::memset(dst, 0, capacity);
        std::memcpy(dst, src.data(), std::min(capacity - 1, src.size()));
    }

    // Simple growing buffer that hands out 16-byte aligned offsets from the start of the file
    class BlobWriter
    {
    public:
        explicit BlobWriter(size_t header_size) { m_Data.resize(header_size, 0); }

        template<typename T>
        uint64_t Append(const std::vector<T>& v)
        {
            return v.empty() ? 0 : Append(v.data(), sizeof(T) * v.size());
        }

        uint64_t Append(const void* data, size_t size)
        {
            while (m_Data.size() % 16 != 0)
                m_Data.push_back(0);

            const uint64_t offset = m_Data.size();
            const uint8_t* bytes = (const uint8_t*)data;
            m_Data.insert(m_Data.end(), bytes, bytes + size);
            return offset;
        }

        template<typename T>
        void WriteHeader(const T& header)
        {
            std::memcpy(m_Data.data(), &header, sizeof(T));
        }

        std::vector<uint8_t>& Data() { return m_Data; }

    private:
        std::vector<uint8_t> m_Data;
    };

    // Writes the bytes to disk, returns false if the file could not be opened
    bool WriteFile(const std::filesystem::path& file, const std::vector<uint8_t>& data);

    // True if the file exists and its contents differ from data
    bool FileDiffers(const std::filesystem::path& file, const std::vector<uint8_t>& data);
}

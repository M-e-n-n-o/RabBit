#include <fstream>
#include <iterator>

#include "Utils.h"

namespace RB::ModelConverter
{
    bool WriteFile(const std::filesystem::path& file, const std::vector<uint8_t>& data)
    {
        std::ofstream stream(file, std::ios::binary);
        if (!stream.is_open())
            return false;

        stream.write((const char*)data.data(), (std::streamsize)data.size());
        return stream.good();
    }

    bool FileDiffers(const std::filesystem::path& file, const std::vector<uint8_t>& data)
    {
        std::error_code ec;
        if (!std::filesystem::exists(file, ec))
            return false;

        if (std::filesystem::file_size(file, ec) != data.size())
            return true;

        std::ifstream stream(file, std::ios::binary);
        std::vector<uint8_t> existing((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        return existing != data;
    }
}

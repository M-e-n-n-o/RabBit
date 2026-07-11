#include <string>
#include <fstream>
#include <filesystem>
#include <vector>

#include "Log.h"
#include "ArgParsing.h"
#include "CompiledTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <squish/squish.h>

using namespace RB::TextureConverter;

const char* FormatNames[] = {
    "Invalid",
    "R8",
    "RGBA8",
    "RGBA8 SRGB",
    "BC1",
    "BC1 SRGB",
    "BC3",
    "BC3 SRGB",
    "BC4",
    "BC5"
};
static_assert(_countof(FormatNames) == kFormat_Count);

std::string GetFilenameWithoutExtension(const char* path)
{
    std::filesystem::path file_path(path);
    return file_path.stem().string();
}

int main(int argc, char* argv[])
{
    LOGW(L"---------------- Starting RabBit's texture converter ----------------");

    auto start_time = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
#ifdef _WIN32
    localtime_s(&local_time, &time);
#else
    localtime_r(&time, &local_time);
#endif

    LOG("Start time: " << std::put_time(&local_time, "%d-%m-%Y %H:%M:%S"));
    LOG("");

    DEFINE_FIND_LAUNCH_ARG(argc, argv);
    DEFINE_HAS_LAUNCH_ARG(argc, argv);

    const char* input_file = FindLaunchArg("-input");
    EXIT_ON_FAIL(input_file != nullptr, "No -input file specified");
    LOG("Input file: " << input_file);

    const char* output_file = FindLaunchArg("-output");
    EXIT_ON_FAIL(output_file != nullptr, "No -output file specified");
    LOG("Output file: " << output_file);

    const char* target_format_name = FindLaunchArg("-format");
    EXIT_ON_FAIL(target_format_name != nullptr, "No -format specified");
        
    int32_t target_format = kFormat_Invalid;
    if (std::strcmp(target_format_name, "R8") == 0)
        target_format = kFormat_R8;
    else if (std::strcmp(target_format_name, "RGBA8") == 0)
        target_format = kFormat_RGBA8;
    else if (std::strcmp(target_format_name, "RGBA8SRGB") == 0)
        target_format = kFormat_RGBA8_SRGB;
    else if (std::strcmp(target_format_name, "BC1") == 0)
        target_format = kFormat_BC1;
    else if (std::strcmp(target_format_name, "BC1SRGB") == 0)
        target_format = kFormat_BC1_SRGB;
    else if (std::strcmp(target_format_name, "BC3") == 0)
        target_format = kFormat_BC3;
    else if (std::strcmp(target_format_name, "BC3SRGB") == 0)
        target_format = kFormat_BC3_SRGB;
    else if (std::strcmp(target_format_name, "BC4") == 0)
        target_format = kFormat_BC4;
    else if (std::strcmp(target_format_name, "BC5") == 0)
        target_format = kFormat_BC5;
    EXIT_ON_FAIL(target_format > kFormat_Invalid, "Format not recognized");
    LOG("Target format: " << FormatNames[target_format]);

    const char* mips_name = FindLaunchArg("-mips");
    uint32_t mip_count = 1;
    if (mips_name)
    {
        if (std::strcmp(mips_name, "max") == 0)
            mip_count = MaxMips;
        else
            mip_count = mips_name[0] - '0';

        LOG("Mip count: " << mip_count);
    }
    else
    {
        LOG("No -mips specified, defaulting to " << mip_count);
    }

    LOG("");

    // Read the input file
    std::ios_base::openmode input_mode = std::fstream::in | std::fstream::binary;
    std::fstream input_stream;
    input_stream.open(input_file, input_mode);
    EXIT_ON_FAIL(input_stream.is_open(), "Could not open input file");

    input_stream.seekg(0, input_stream.end);
    uint32_t file_length = input_stream.tellg();
    input_stream.seekg(0, input_stream.beg);

    uint8_t* file_buffer = new uint8_t[file_length];
    input_stream.read((char*)file_buffer, file_length);

    LOG("Loaded texture from disk with size: " << (file_length / (1024.0f * 1024.0f)) << " MiB");

    // Load image using STB
    int32_t width = 0;
    int32_t height = 0;
    int32_t actual_channels = 0;
    uint32_t texture_size = 0;
    uint8_t* texture_memory = nullptr;
    {
        bool success = stbi_info_from_memory((stbi_uc*)file_buffer, file_length, &width, &height, &actual_channels);
        if (!success)
        {
            const char* error_msg = stbi_failure_reason();
            EXIT_ON_FAIL(false, "Failed to load info of texture, error message: " << error_msg);
        }

        if ((target_format == kFormat_R8 || target_format == kFormat_BC4) && actual_channels != 1)
        {
            actual_channels = 1;
            LOG("Input texture does not have 1 channel, forcing it...");
        } 
        else if ((target_format == kFormat_RGBA8 || target_format == kFormat_RGBA8_SRGB) && actual_channels != 4)
        {
            actual_channels = 4;
            LOG("Input texture does not have 4 channels, forcing it...");
        }
        else if ((target_format == kFormat_BC1 || target_format == kFormat_BC1_SRGB || target_format == kFormat_BC3 || target_format == kFormat_BC3_SRGB) 
            && (actual_channels != 3 && actual_channels != 4))
        {
            actual_channels = 4;
            LOG("Input texture does not have 3 or 4 channels, forcing it...");
        }
        else if (target_format == kFormat_BC5 && actual_channels != 2)
        {
            actual_channels = 2;
            LOG("Input texture does not have 2 channels, forcing it...");
        }

        // !!! NOTE: This loads a 8 bit per channel image (use stbi_load_16_from_memory or stbi_loadf_from_memory for 16 or 32 bit) !!!
        int32_t original_channels;
        texture_memory = stbi_load_from_memory((stbi_uc*)file_buffer, file_length, &width, &height, &original_channels, actual_channels);
        if (texture_memory == NULL)
        {
            const char* error_msg = stbi_failure_reason();
            EXIT_ON_FAIL(false, "Failed to load texture, error message: " << error_msg);
        }

        texture_size = size_t(width) * size_t(height) * size_t(actual_channels);
        LOG("Decoded texture with size: " << (texture_size / (1024.0f * 1024.0f)) << " MiB");
    }

    input_stream.close();
    delete[] file_buffer;

    // Compress
    int32_t compressed_size = 0;
    uint8_t* compressed_memory = nullptr;
    if (target_format != kFormat_R8 && target_format != kFormat_RGBA8 && target_format != kFormat_RGBA8_SRGB)
    {
        int flags = squish::kColourIterativeClusterFit;
        switch (target_format)
        {
        case kFormat_BC1: 
        case kFormat_BC1_SRGB:  flags |= squish::kDxt1; break;
        case kFormat_BC3:       
        case kFormat_BC3_SRGB:  flags |= squish::kDxt5; break;
        case kFormat_BC4:       flags |= squish::kBc4; break;
        case kFormat_BC5:       flags |= squish::kBc5; break;
        default:
            EXIT_ON_FAIL(false, "libsquish does not support target format");
            break;
        }

        compressed_size = squish::GetStorageRequirements(width, height, flags);

        compressed_memory = new uint8_t[compressed_size];
        squish::CompressImage(texture_memory, width, height, compressed_memory, flags);

        LOG("Compressed image to size: " << (compressed_size / (1024.0f * 1024.0f)) << " MiB");
    }

    // Write the output file
    {
        std::ios_base::openmode output_mode = std::fstream::out | std::fstream::binary;
        std::fstream output_stream;
        output_stream.open(output_file, output_mode);
        EXIT_ON_FAIL(output_stream.is_open(), "Could not open output file");

        std::string output_name = GetFilenameWithoutExtension(output_file);

        CompiledTextureHeader header = {};
        header.magic            = ValidMagic;
        header.format           = target_format;
        header.targetMips       = mip_count;
        header.width            = width;
        header.height           = height;
        header.dataSize         = compressed_memory ? compressed_size : texture_size;
        memset(header.name, 0, _countof(header.name));
        memcpy(header.name, output_name.data(), std::min(_countof(header.name), output_name.size()));

        output_stream.write((char*)&header, sizeof(CompiledTextureHeader));

        if (compressed_memory)
            output_stream.write((char*)compressed_memory, compressed_size);
        else
            output_stream.write((char*)texture_memory, texture_size);

        output_stream.close();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> delta = end_time - start_time;

    LOGW(L"");
    LOGW(L"Conversion time: " << delta.count() << " ms");
    LOGW(L"---------------------------------------------------------------------");
    LOGW(L"Succesfully finished writing to the output file");

    if (compressed_memory)
        delete[] compressed_memory;
    stbi_image_free(texture_memory);

    return 0;
}
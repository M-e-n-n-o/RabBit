#include <string>
#include <fstream>
#include <filesystem>
#include <vector>

#include "Log.h"
#include "ArgParsing.h"
#include "CompiledTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize2.h>

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

const bool IsSRGB[] = {
    false, // Invalid
    false, // R8
    false, // RGBA8
    true,  // RGBA8 SRGB
    false, // BC1
    true,  // BC1 SRGB
    false, // BC3
    true,  // BC3 SRGB
    false, // BC4
    false  // BC5
};
static_assert(_countof(IsSRGB) == kFormat_Count);

struct MipTexture
{
    int width;
    int height;
    uint8_t* data;
    uint32_t size;
};

std::vector<MipTexture> GenerateMipChain(const MipTexture& base, uint32_t* in_out_mips, bool is_srgb, uint32_t channels, bool tiled_texture, bool normal_texture);

std::string GetFilenameWithoutExtension(const char* path);

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
        
    const bool is_tiled = HasLaunchArg("-tiled");
    LOG("Is tiled texture: " << is_tiled);

    const bool is_normal = HasLaunchArg("-normal");
    LOG("Contains normals: " << is_normal);

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
        {
            mip_count = UINT32_MAX;
            LOG("Mip count: Maximum");
        }
        else
        {
            mip_count = mips_name[0] - '0';
            LOG("Mip count: " << mip_count);
        }
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
    int32_t channels = 0;
    uint32_t texture_size = 0;
    uint8_t* texture_memory = nullptr;
    {
        bool success = stbi_info_from_memory((stbi_uc*)file_buffer, file_length, &width, &height, &channels);
        if (!success)
        {
            const char* error_msg = stbi_failure_reason();
            EXIT_ON_FAIL(false, "Failed to load info of texture, error message: " << error_msg);
        }

        if ((target_format == kFormat_R8 || target_format == kFormat_BC4) && channels != 1)
        {
            channels = 1;
            LOG("Input texture does not have 1 channel, forcing it...");
        } 
        else if ((target_format == kFormat_RGBA8 || target_format == kFormat_RGBA8_SRGB) && channels != 4)
        {
            channels = 4;
            LOG("Input texture does not have 4 channels, forcing it...");
        }
        else if ((target_format == kFormat_BC1 || target_format == kFormat_BC1_SRGB || target_format == kFormat_BC3 || target_format == kFormat_BC3_SRGB) 
            && (channels != 3 && channels != 4))
        {
            channels = 4;
            LOG("Input texture does not have 3 or 4 channels, forcing it...");
        }
        else if (target_format == kFormat_BC5 && channels != 2)
        {
            channels = 2;
            LOG("Input texture does not have 2 channels, forcing it...");
        }

        // !!! NOTE: This loads a 8 bit per channel image (use stbi_load_16_from_memory or stbi_loadf_from_memory for 16 or 32 bit) !!!
        int32_t original_channels;
        texture_memory = stbi_load_from_memory((stbi_uc*)file_buffer, file_length, &width, &height, &original_channels, channels);
        if (texture_memory == NULL)
        {
            const char* error_msg = stbi_failure_reason();
            EXIT_ON_FAIL(false, "Failed to load texture, error message: " << error_msg);
        }

        texture_size = size_t(width) * size_t(height) * size_t(channels);
        LOG("Decoded texture with size: " << (texture_size / (1024.0f * 1024.0f)) << " MiB");
    }

    input_stream.close();
    delete[] file_buffer;

    // Generate mips
    std::vector<MipTexture> mips;
    {
        MipTexture base_mip = {};
        base_mip.width  = width;
        base_mip.height = height;
        base_mip.data   = texture_memory;
        base_mip.size   = texture_size;

        mips = GenerateMipChain(base_mip, &mip_count, IsSRGB[target_format], channels, is_tiled, is_normal);

        LOG("Generated mips: " << mip_count);

        // Need to calculate the final texture size uncompressed
        texture_size = 0;
        for (const MipTexture& mip : mips)
        {
            texture_size += mip.size;
        }

        LOG("Size after mip generation: " << (texture_size / (1024.0f * 1024.0f)) << " MiB");
    }

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

        int32_t* compressed_sizes = (int32_t*)alloca(sizeof(int32_t) * mips.size());
        for (int i = 0; i < mips.size(); i++)
        {
            const MipTexture& mip = mips[i];
            compressed_sizes[i] = squish::GetStorageRequirements(mip.width, mip.height, flags);
            compressed_size += compressed_sizes[i];
        }

        compressed_memory = new uint8_t[compressed_size];

        int32_t offset = 0;
        for (int i = 0; i < mips.size(); i++)
        {
            const MipTexture& mip = mips[i];
            squish::CompressImage(mip.data, mip.width, mip.height, compressed_memory + offset, flags);
            offset += compressed_sizes[i];
        }

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
        header.mipCount         = mip_count;
        header.width            = width;
        header.height           = height;
        header.dataSize         = compressed_memory ? compressed_size : texture_size;
        memset(header.name, 0, _countof(header.name));
        memcpy(header.name, output_name.data(), std::min(_countof(header.name), output_name.size()));

        output_stream.write((char*)&header, sizeof(CompiledTextureHeader));

        if (compressed_memory)
        {
            output_stream.write((char*)compressed_memory, compressed_size);
        }
        else
        {
            for (int i = 0; i < mips.size(); i++)
            {
                const MipTexture& mip = mips[i];
                output_stream.write((char*)mip.data, mip.size);
            }
        }

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
    for (int i = 1; i < mips.size(); i++)
        delete[] mips[i].data;
    stbi_image_free(texture_memory);

    return 0;
}

MipTexture GenerateMip(const MipTexture& src, uint32_t channels, stbir_pixel_layout layout, 
    stbir_datatype data_type, stbir_edge edge, stbir_filter filter)
{
    MipTexture dst;

    dst.width = std::max(1, src.width / 2);
    dst.height = std::max(1, src.height / 2);

    dst.size = dst.width * dst.height * channels;
    dst.data = new uint8_t[dst.size];

    stbir_resize(src.data,
                 src.width,
                 src.height,
                 0,
                 dst.data,
                 dst.width,
                 dst.height,
                 0,
                 layout,
                 data_type,
                 edge,
                 filter);

    // TODO: Do renormalization on normal maps (also generate each mip from the top level rather than chaining to avoid renormalization errors)

    return dst;
}

std::vector<MipTexture> GenerateMipChain(const MipTexture& base, uint32_t* in_out_mips, bool is_srgb, uint32_t channels, bool tiled_texture, bool normal_texture)
{
    std::vector<MipTexture> result;
    result.push_back(base);

    stbir_pixel_layout layout;
    switch (channels)
    {
    case 1: layout = STBIR_1CHANNEL; break;
    case 2: layout = STBIR_2CHANNEL; break;
    case 3: layout = STBIR_RGB; break;
    case 4: layout = STBIR_RGBA; break;
    default:
        EXIT_ON_FAIL(false, "Mip chain generation does not support " << channels << " amount of channels");
    }

    stbir_datatype data_type = STBIR_TYPE_UINT8;
    if (is_srgb)
        data_type = STBIR_TYPE_UINT8_SRGB;
    
    stbir_edge edge = STBIR_EDGE_CLAMP;
    if (tiled_texture)
        edge = STBIR_EDGE_WRAP; // For example terrain textures

    stbir_filter filter = STBIR_FILTER_MITCHELL;
    if (normal_texture)
        filter = STBIR_FILTER_CUBICBSPLINE;

    MipTexture current = base;
    while ((current.width > 1 || current.height > 1) && result.size() < *in_out_mips)
    {
        MipTexture next = GenerateMip(current, channels, layout, data_type, edge, filter);
        result.push_back(next);
        current = next;
    }

    *in_out_mips = result.size();

    return result;
}

std::string GetFilenameWithoutExtension(const char* path)
{
    std::filesystem::path file_path(path);
    return file_path.stem().string();
}
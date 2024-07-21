#include "RabBitCommon.h"
#include "AssetManager.h"
#include "utils/File.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace RB::Graphics
{
	LoadedImage::LoadedImage()
		: data(nullptr)
	{}

	LoadedImage::~LoadedImage()
	{
		stbi_image_free(data);
		data = nullptr;
	}

	namespace AssetManager
	{
		static const char* g_AssetPath = nullptr;

		void Init(const char* asset_path)
		{
			g_AssetPath = asset_path;
		}

		bool LoadImage8Bit(const char* path, LoadedImage* out_image, uint32_t force_channels)
		{
			std::string final_path = (((std::string)g_AssetPath) + ((std::string)path));

			auto file_handle = FileLoader::OpenFile(final_path.c_str(), OpenFileMode::kFileModeRead | OpenFileMode::kFileModeBinary);

			FileData data = file_handle->ReadFull();

			// Note that this loads a 8 bit per channel image (use stbi_load_16_from_memory or stbi_loadf_from_memory for 16 or 32 bit)
			out_image->data = stbi_load_from_memory((stbi_uc*)data.data, data.size, &out_image->width, &out_image->height, &out_image->channels, force_channels);

			if (out_image->data == NULL)
			{
				const char* error_msg = stbi_failure_reason();
				RB_LOG_ERROR(LOGTAG_GRAPHICS, "Failed to load texture \"%s\" with STB, error message: %s", final_path.c_str(), error_msg);
				return false;
			}

			if (force_channels != 0)
			{
				// We forced to read only certain channels
				out_image->channels = force_channels;
			}

			switch (out_image->channels)
			{
			case 1: out_image->format = RenderResourceFormat::R8_UNORM; break;
			case 4: out_image->format = RenderResourceFormat::R8G8B8A8_UNORM; break;
			case 0:
			case 2:
			case 3:
			default:
				RB_LOG_ERROR(LOGTAG_GRAPHICS, "This many channels for an 8 bit image is not supported");
				out_image->format = RenderResourceFormat::Unkown;
				break;
			}

			out_image->dataSize = GetElementSizeFromFormat(out_image->format) * out_image->width * out_image->height;

			return true;
		}
	}
}
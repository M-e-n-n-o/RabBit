#include "RabBitCommon.h"
#include "Mesh.h"

#include "utils/File.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace RB::Entity
{
	Material::Material(const char* file_name)
		: m_Texture(nullptr)
	{
		auto file_handle = FileLoader::OpenFile(file_name, OpenFileMode::kFileModeRead | OpenFileMode::kFileModeBinary);

		FileData data = file_handle->ReadFull();

		int32_t img_width, img_height, img_channel_count;

		// Note that this loads a 8 bit per channel image (use stbi_load_16_from_memory or stbi_loadf_from_memory for 16 or 32 bit)
		// The last parameter can force the amount of channels read from the texture!
		uint8_t* img_data = stbi_load_from_memory((stbi_uc*)data.data, data.size, &img_width, &img_height, &img_channel_count, 0);

		if (img_data == NULL)
		{
			const char* error_msg = stbi_failure_reason();
			RB_LOG_ERROR(LOGTAG_GRAPHICS, "Failed to load texture \"%s\" with STB, error message: %s", file_name, error_msg);
			return;
		}

		// The data size is probably wrong here
		static_assert(false);

		m_Texture = Graphics::Texture2D::Create("Cool Texture", img_data, data.size, Graphics::RenderResourceFormat::R8G8B8A8_UNORM, img_width, img_height, false, false, false);

		stbi_image_free(img_data);
	}
}
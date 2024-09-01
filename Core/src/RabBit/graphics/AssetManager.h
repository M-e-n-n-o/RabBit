#pragma once

#include "RabBitCommon.h"
#include "RenderResource.h"

#if defined(LoadImage)
#undef LoadImage
#endif

namespace RB::Graphics
{
    struct LoadedImage
    {
        void*                   data;
        uint32_t				dataSize;
        RenderResourceFormat	format;
        int32_t					width;
        int32_t					height;
        int32_t					channels;

        LoadedImage();
        ~LoadedImage();
    };

    struct LoadedModel
    {
        uint32_t*               vertices;
        uint32_t                verticesCount;
        uint32_t*               uvs;
        uint32_t                uvsCount;
        uint16_t*               indices;
        uint32_t                indicesCount;
        void*                   internalScene;

        LoadedModel();
        ~LoadedModel();
    };

    namespace AssetManager
    {
        void Init(const char* asset_base_path);

        bool LoadImage8Bit(const char* path, LoadedImage* out_image, uint32_t force_channels = 0);

        bool LoadModel(const char* path, LoadedModel* out_model);
    }
}
#include "RabBitCommon.h"
#include "Mesh.h"
#include "graphics/AssetManager.h"

namespace RB::Entity
{
    Material::Material(const char* file_name)
        : m_Texture(nullptr)
    {
        Graphics::LoadedImage img;
        bool success = Graphics::AssetManager::LoadImage8Bit(file_name, &img);

        if (success)
        {
            m_Texture = Graphics::Texture2D::Create("Cool Texture", img.data, img.dataSize, img.format, img.width, img.height, false, false, false);
        }
    }
}
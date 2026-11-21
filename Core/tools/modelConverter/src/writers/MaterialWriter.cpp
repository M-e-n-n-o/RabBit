#include "Writers.h"
#include "Utils.h"
#include "Log.h"

namespace RB::ModelConverter
{
    namespace
    {
        std::string ApplyExtension(const std::string& texture, const std::string& extension)
        {
            if (texture.empty() || extension.empty())
                return texture;

            return std::filesystem::path(texture).replace_extension(extension).string();
        }

        const char* ToString(BlendMode mode)
        {
            switch (mode)
            {
            case BlendMode::Opaque:  return "opaque";
            case BlendMode::Blended: return "blended";
            case BlendMode::Cutout:  return "cutout";
            }
            return "unknown";
        }
    }

    bool WriteMaterial(const std::filesystem::path& file, const std::string& name, const ImportedMaterial& material, const std::string& texture_extension)
    {
        const std::string diffuse = ApplyExtension(material.diffuseTexture, texture_extension);
        const std::string normal  = ApplyExtension(material.normalTexture, texture_extension);

        if (diffuse.size() >= kMaxTextureNameLength || normal.size() >= kMaxTextureNameLength)
            LOG("Warning: texture name of material \"" << name << "\" is longer than " << (kMaxTextureNameLength - 1) << " characters and will be truncated");

        CompiledMaterial compiled = {};
        compiled.magic     = kMaterialMagic;
        compiled.version   = kMaterialVersion;
        compiled.blendMode = material.blendMode;
        CopyName(compiled.name, sizeof(compiled.name), name);
        CopyName(compiled.diffuseTexture, sizeof(compiled.diffuseTexture), diffuse);
        CopyName(compiled.normalTexture, sizeof(compiled.normalTexture), normal);

        const uint8_t* bytes = (const uint8_t*)&compiled;
        const std::vector<uint8_t> data(bytes, bytes + sizeof(compiled));

        // Materials of different models share a folder, warn when one replaces a different material with the same name
        if (FileDiffers(file, data))
            LOG("Warning: overwriting existing material file with different contents: " << file.filename().string());

        if (!WriteFile(file, data))
        {
            LOG("Could not write material file: " << file.string());
            return false;
        }

        LOG("Wrote material: " << file.filename().string() << " (" << ToString(material.blendMode) << ", diffuse \"" << diffuse << "\", normal \"" << normal << "\")");
        return true;
    }
}

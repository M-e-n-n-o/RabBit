#include <algorithm>
#include <cctype>
#include <string>

#include "Importer.h"
#include "FbxImporter.h"

namespace RB::ModelConverter
{
    std::unique_ptr<Importer> CreateImporter(const std::filesystem::path& file)
    {
        std::string ext = file.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });

        if (ext == ".fbx")
            return std::make_unique<FbxImporter>();

        return nullptr;
    }
}

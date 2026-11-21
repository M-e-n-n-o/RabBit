#pragma once

#include "Importer.h"

namespace RB::ModelConverter
{
    class FbxImporter : public Importer
    {
    public:
        bool Import(const std::filesystem::path& file, const ImportOptions& options, ImportedScene& out) override;
    };
}

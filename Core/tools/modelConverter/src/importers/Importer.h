#pragma once

#include <filesystem>
#include <memory>

#include "ImportedScene.h"

namespace RB::ModelConverter
{
    // Base for every source format. To add a format: derive from this and register it in ImporterRegistry.cpp
    class Importer
    {
    public:
        virtual ~Importer() = default;

        virtual bool Import(const std::filesystem::path& file, const ImportOptions& options, ImportedScene& out) = 0;
    };

    // Picks an importer based on the file extension, nullptr if the format is not supported
    std::unique_ptr<Importer> CreateImporter(const std::filesystem::path& file);
}

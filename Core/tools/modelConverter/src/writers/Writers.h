#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "ImportedScene.h"

namespace RB::ModelConverter
{
    // .rbms: geometry, node hierarchy and animations. Knows nothing about materials
    bool WriteMesh(const std::filesystem::path& file, const std::string& name, const ImportedScene& scene);

    // .rbmt: blend mode and texture references. texture_extension replaces the source texture extension if not empty
    bool WriteMaterial(const std::filesystem::path& file, const std::string& name, const ImportedMaterial& material, const std::string& texture_extension);

    // .rbmd: references the mesh file and the material files, plus which material each submesh uses.
    // material_files is indexed like scene.materials
    bool WriteModel(const std::filesystem::path& file, const std::string& name, const std::string& mesh_file,
        const std::vector<std::string>& material_files, const ImportedScene& scene);
}

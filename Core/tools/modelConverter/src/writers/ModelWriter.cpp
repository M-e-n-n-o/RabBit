#include "Writers.h"
#include "Utils.h"
#include "Log.h"
#include "CompiledModel.h"

namespace RB::ModelConverter
{
    bool WriteModel(const std::filesystem::path& file, const std::string& name, const std::string& mesh_file,
        const std::vector<std::string>& material_files, const ImportedScene& scene)
    {
        BlobWriter blob(sizeof(CompiledModelHeader));

        std::vector<CompiledModelMaterial> materials(material_files.size());
        for (size_t i = 0; i < material_files.size(); i++)
            CopyName(materials[i].file, sizeof(materials[i].file), material_files[i]);

        std::vector<uint32_t> submesh_materials;
        submesh_materials.reserve(scene.submeshes.size());
        for (const ImportedSubmesh& s : scene.submeshes)
            submesh_materials.push_back(s.materialIndex < material_files.size() ? s.materialIndex : kNoMaterial);

        CompiledModelHeader header = {};
        header.magic                  = kModelMagic;
        header.version                = kModelVersion;
        CopyName(header.name, sizeof(header.name), name);
        CopyName(header.meshFile, sizeof(header.meshFile), mesh_file);
        header.materialCount          = (uint32_t)materials.size();
        header.submeshCount           = (uint32_t)submesh_materials.size();
        header.materialsOffset        = blob.Append(materials);
        header.submeshMaterialsOffset = blob.Append(submesh_materials);
        header.fileSize               = blob.Data().size();
        blob.WriteHeader(header);

        if (!WriteFile(file, blob.Data()))
        {
            LOG("Could not write model file: " << file.string());
            return false;
        }

        LOG("Wrote model: " << file.filename().string() << " (" << materials.size() << " materials, " << submesh_materials.size() << " submeshes)");
        return true;
    }
}

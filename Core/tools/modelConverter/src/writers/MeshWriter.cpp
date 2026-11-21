#include "Writers.h"
#include "Utils.h"
#include "Log.h"

namespace RB::ModelConverter
{
    bool WriteMesh(const std::filesystem::path& file, const std::string& name, const ImportedScene& scene)
    {
        BlobWriter blob(sizeof(CompiledMeshHeader));

        // Submeshes: bulk arrays first, then the table that points at them
        std::vector<CompiledSubmesh> submesh_table;
        for (const ImportedSubmesh& s : scene.submeshes)
        {
            CompiledSubmesh c = {};
            c.nodeIndex     = s.nodeIndex;
            c.vertexCount   = (uint32_t)s.positions.size();
            c.indexCount    = (uint32_t)s.indices.size();
            c.boneCount     = (uint32_t)s.skinBones.size();
            c.flags         = s.flags;
            c.minBounds     = s.minBounds;
            c.maxBounds     = s.maxBounds;

            c.positionsOffset    = blob.Append(s.positions);
            c.verticesOffset     = blob.Append(s.vertices);
            c.indicesOffset      = blob.Append(s.indices);
            c.skinVerticesOffset = blob.Append(s.skinVertices);
            c.skinBonesOffset    = blob.Append(s.skinBones);

            submesh_table.push_back(c);
        }

        // Animations: key arrays, then channel tables, then the animation table
        std::vector<CompiledAnimation> animation_table;
        for (const ImportedAnimation& a : scene.animations)
        {
            std::vector<CompiledChannel> channel_table;
            for (const ImportedChannel& ch : a.channels)
            {
                CompiledChannel c = {};
                c.nodeIndex             = ch.nodeIndex;
                c.translationKeyCount   = (uint32_t)ch.translation.size();
                c.rotationKeyCount      = (uint32_t)ch.rotation.size();
                c.scaleKeyCount         = (uint32_t)ch.scale.size();
                c.translationKeysOffset = blob.Append(ch.translation);
                c.rotationKeysOffset    = blob.Append(ch.rotation);
                c.scaleKeysOffset       = blob.Append(ch.scale);
                channel_table.push_back(c);
            }

            CompiledAnimation c = {};
            CopyName(c.name, sizeof(c.name), a.name);
            c.duration       = a.duration;
            c.channelCount   = (uint32_t)channel_table.size();
            c.channelsOffset = blob.Append(channel_table);
            animation_table.push_back(c);
        }

        CompiledMeshHeader header = {};
        header.magic            = kMeshMagic;
        header.version          = kMeshVersion;
        CopyName(header.name, sizeof(header.name), name);
        header.submeshCount     = (uint32_t)submesh_table.size();
        header.nodeCount        = (uint32_t)scene.nodes.size();
        header.animationCount   = (uint32_t)animation_table.size();
        header.submeshesOffset  = blob.Append(submesh_table);
        header.nodesOffset      = blob.Append(scene.nodes);
        header.animationsOffset = blob.Append(animation_table);
        header.fileSize         = blob.Data().size();
        blob.WriteHeader(header);

        if (!WriteFile(file, blob.Data()))
        {
            LOG("Could not write mesh file: " << file.string());
            return false;
        }

        LOG("Wrote mesh: " << file.filename().string() << " (" << (blob.Data().size() / (1024.0f * 1024.0f)) << " MiB)");
        return true;
    }
}

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

#include "Log.h"
#include "ArgParsing.h"
#include "CompiledModel.h"

#include <ufbx.h>

using namespace RB::ModelConverter;

// ------------------------------------------------------------------------------------------------
// Intermediate data (what is gathered from ufbx, before serializing)
// ------------------------------------------------------------------------------------------------

struct ConvertedSubmodel
{
    uint32_t                        nodeIndex       = kInvalidIndex;
    uint32_t                        materialIndex   = kInvalidIndex;
    uint32_t                        flags           = 0;
    PackedFloat3                    minBounds       = {};
    PackedFloat3                    maxBounds       = {};
    std::vector<PackedFloat3>       positions;
    std::vector<CompiledVertex>     vertices;
    std::vector<uint32_t>           indices;
    std::vector<CompiledSkinVertex> skinVertices;
    std::vector<CompiledSkinBone>   skinBones;
};

struct ConvertedChannel
{
    uint32_t                    nodeIndex = 0;
    std::vector<CompiledKey>    translation;
    std::vector<CompiledRotationKey> rotation;
    std::vector<CompiledKey>    scale;
};

struct ConvertedAnimation
{
    std::string                     name;
    float                           duration = 0.0f;
    std::vector<ConvertedChannel>   channels;
};

// ------------------------------------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------------------------------------

namespace
{
    void CopyName(char* dst, size_t capacity, const std::string& src)
    {
        std::memset(dst, 0, capacity);
        std::memcpy(dst, src.data(), std::min(capacity - 1, src.size()));
    }

    PackedFloat3 ToFloat3(const ufbx_vec3& v)
    {
        return { (float)v.x, (float)v.y, (float)v.z };
    }

    // Row-major (element (row, col) at [row * 4 + col]), column vector convention: the translation is in the last column
    void ToFloat4x4(const ufbx_matrix& m, float out[16])
    {
        out[0]  = (float)m.m00; out[1]  = (float)m.m01; out[2]  = (float)m.m02; out[3]  = (float)m.m03;
        out[4]  = (float)m.m10; out[5]  = (float)m.m11; out[6]  = (float)m.m12; out[7]  = (float)m.m13;
        out[8]  = (float)m.m20; out[9]  = (float)m.m21; out[10] = (float)m.m22; out[11] = (float)m.m23;
        out[12] = 0.0f;         out[13] = 0.0f;         out[14] = 0.0f;         out[15] = 1.0f;
    }

    PackedFloat4 ToFloat4(const ufbx_quat& q)
    {
        return { (float)q.x, (float)q.y, (float)q.z, (float)q.w };
    }

    float Dot(const PackedFloat4& a, const PackedFloat4& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    std::string GetFilenameWithoutExtension(const char* path)
    {
        return std::filesystem::path(path).stem().string();
    }

    // Simple growing buffer that hands out 16-byte aligned offsets from the start of the file
    class BlobWriter
    {
    public:
        explicit BlobWriter(size_t header_size) { m_Data.resize(header_size, 0); }

        template<typename T>
        uint64_t Append(const std::vector<T>& v)
        {
            return v.empty() ? 0 : Append(v.data(), sizeof(T) * v.size());
        }

        uint64_t Append(const void* data, size_t size)
        {
            while (m_Data.size() % 16 != 0)
                m_Data.push_back(0);

            const uint64_t offset = m_Data.size();
            const uint8_t* bytes = (const uint8_t*)data;
            m_Data.insert(m_Data.end(), bytes, bytes + size);
            return offset;
        }

        std::vector<uint8_t>& Data() { return m_Data; }

    private:
        std::vector<uint8_t> m_Data;
    };
}

// ------------------------------------------------------------------------------------------------
// Conversion
// ------------------------------------------------------------------------------------------------

static std::string GetTextureName(const ufbx_material* material, const std::string& texture_extension)
{
    std::string name;
    if (material->fbx.diffuse_color.texture)
        name = std::string(material->fbx.diffuse_color.texture->filename.data, material->fbx.diffuse_color.texture->filename.length);
    else
        name = std::string(material->name.data, material->name.length); // Same fallback as before: use the material name

    // Strip the directory, the engine finds textures by file name
    const size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos)
        name = name.substr(slash + 1);

    if (!texture_extension.empty())
        name = std::filesystem::path(name).replace_extension(texture_extension).string();

    return name;
}

static CompiledSkinVertex MakeSkinVertex(const ufbx_skin_deformer* skin, uint32_t position_vertex_index)
{
    CompiledSkinVertex out = {};

    // Skin data is per position vertex, not per face corner. Weights are sorted strongest first by ufbx
    const ufbx_skin_vertex sv = skin->vertices.data[position_vertex_index];
    const uint32_t count = std::min<uint32_t>(sv.num_weights, kMaxInfluences);

    float total = 0.0f;
    for (uint32_t w = 0; w < count; w++)
    {
        const ufbx_skin_weight sw = skin->weights.data[sv.weight_begin + w];
        out.bones[w]   = (uint16_t)sw.cluster_index;
        out.weights[w] = (float)sw.weight;
        total += out.weights[w];
    }

    if (total > 0.0f)
    {
        for (uint32_t w = 0; w < kMaxInfluences; w++)
            out.weights[w] /= total;
    }
    else
    {
        // Vertex without any weights, bind it fully to the first bone so it doesn't collapse to the origin
        out.weights[0] = 1.0f;
    }

    return out;
}

static bool ConvertMeshPart(const ufbx_mesh* mesh, const ufbx_mesh_part* part, const ufbx_node* node, ConvertedSubmodel& out)
{
    out = ConvertedSubmodel{};
    out.nodeIndex = (uint32_t)node->typed_id;

    if (part->index < mesh->materials.count && mesh->materials.data[part->index] != nullptr)
        out.materialIndex = (uint32_t)mesh->materials.data[part->index]->typed_id;

    const ufbx_skin_deformer* skin = mesh->skin_deformers.count > 0 ? mesh->skin_deformers.data[0] : nullptr;
    if (skin && skin->clusters.count == 0)
        skin = nullptr;

    // The vertices stay in the local space of the node, the node's transform (and its parents') is applied at runtime
    // through the node hierarchy
    if (skin)
    {
        out.flags |= kSubmodel_Skinned;

        out.skinBones.resize(skin->clusters.count);
        for (size_t i = 0; i < skin->clusters.count; i++)
        {
            const ufbx_skin_cluster* cluster = skin->clusters.data[i];
            out.skinBones[i].nodeIndex = cluster->bone_node ? (uint32_t)cluster->bone_node->typed_id : kInvalidIndex;
            ToFloat4x4(cluster->geometry_to_bone, out.skinBones[i].geometryToBone);
        }
    }

    const size_t num_vertices = part->num_triangles * 3;
    std::vector<PackedFloat3>       positions(num_vertices);
    std::vector<CompiledVertex>     vertices(num_vertices);
    std::vector<CompiledSkinVertex> skin_vertices;
    if (skin)
        skin_vertices.resize(num_vertices);

    std::vector<uint32_t> tri_indices(mesh->max_face_triangles * 3);

    PackedFloat3 min_bounds = { +FLT_MAX, +FLT_MAX, +FLT_MAX };
    PackedFloat3 max_bounds = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    size_t vi_global = 0;
    // First fetch all vertices into a flat non-indexed buffer, we also need to triangulate the faces
    for (size_t fi = 0; fi < part->num_faces; fi++)
    {
        const ufbx_face face = mesh->faces.data[part->face_indices.data[fi]];
        const size_t num_tris = ufbx_triangulate_face(tri_indices.data(), tri_indices.size(), mesh, face);

        const ufbx_vec3 default_normal = { 0, 0, 1 };
        const ufbx_vec2 default_uv     = { 0, 0 };

        // Iterate through every vertex of every triangle in the triangulated result
        for (size_t vi = 0; vi < num_tris * 3; vi++)
        {
            const uint32_t ix = tri_indices[vi];

            const ufbx_vec3 pos    = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
            const ufbx_vec3 normal = mesh->vertex_normal.exists ? ufbx_get_vertex_vec3(&mesh->vertex_normal, ix) : default_normal;
            const ufbx_vec2 uv     = mesh->vertex_uv.exists ? ufbx_get_vertex_vec2(&mesh->vertex_uv, ix) : default_uv;

            const PackedFloat3 p = { (float)pos.x, (float)pos.y, (float)pos.z };

            positions[vi_global] = p;
            vertices[vi_global].normal = { (float)normal.x, (float)normal.y, (float)normal.z };
            vertices[vi_global].uv     = { (float)uv.x, 1.0f - (float)uv.y }; // Flip the Y as UFBX uses bottom-left convention

            if (skin)
                skin_vertices[vi_global] = MakeSkinVertex(skin, mesh->vertex_indices.data[ix]);

            min_bounds.x = std::min(min_bounds.x, p.x);
            min_bounds.y = std::min(min_bounds.y, p.y);
            min_bounds.z = std::min(min_bounds.z, p.z);
            max_bounds.x = std::max(max_bounds.x, p.x);
            max_bounds.y = std::max(max_bounds.y, p.y);
            max_bounds.z = std::max(max_bounds.z, p.z);

            vi_global++;
        }
    }

    if (vi_global != num_vertices || num_vertices == 0)
    {
        LOG("Warning: skipping mesh part with unexpected vertex count (" << vi_global << " vs " << num_vertices << ")");
        return false;
    }

    // Optimize the flat vertex buffer into an indexed one. Skin data is a stream as well so only
    // vertices with identical bone data get merged
    std::vector<ufbx_vertex_stream> streams;
    streams.push_back({ positions.data(), positions.size(), sizeof(PackedFloat3) });
    streams.push_back({ vertices.data(),  vertices.size(),  sizeof(CompiledVertex) });
    if (skin)
        streams.push_back({ skin_vertices.data(), skin_vertices.size(), sizeof(CompiledSkinVertex) });

    std::vector<uint32_t> indices(num_vertices);

    // `ufbx_generate_indices` compacts the vertex buffers and returns the number of used vertices
    ufbx_error error;
    const size_t num_compacted_vertices = ufbx_generate_indices(streams.data(), streams.size(), indices.data(), indices.size(), nullptr, &error);
    if (error.type != UFBX_ERROR_NONE)
    {
        LOG("Failed to generate index buffer with ufbx, error message: " << error.description.data);
        return false;
    }

    positions.resize(num_compacted_vertices);
    vertices.resize(num_compacted_vertices);
    if (skin)
        skin_vertices.resize(num_compacted_vertices);

    out.positions    = std::move(positions);
    out.vertices     = std::move(vertices);
    out.indices      = std::move(indices);
    out.skinVertices = std::move(skin_vertices);
    out.minBounds    = min_bounds;
    out.maxBounds    = max_bounds;

    return true;
}

static void ConvertAnimations(const ufbx_scene* scene, double sample_rate, std::vector<ConvertedAnimation>& out)
{
    for (const ufbx_anim_stack* stack : scene->anim_stacks)
    {
        // Bake the stack into local TRS keyframes per node, much easier to consume at runtime than raw FBX curves
        ufbx_bake_opts bake_opts = {};
        bake_opts.resample_rate = sample_rate; // Used for non-linear curves / rotation order conversion

        ufbx_error error;
        ufbx_baked_anim* baked = ufbx_bake_anim(scene, stack->anim, &bake_opts, &error);
        if (baked == nullptr)
        {
            LOG("Failed to bake animation \"" << stack->name.data << "\": " << error.description.data);
            continue;
        }

        ConvertedAnimation anim;
        anim.name     = std::string(stack->name.data, stack->name.length);
        anim.duration = (float)baked->playback_duration;

        const double t0 = baked->playback_time_begin;

        for (const ufbx_baked_node& bn : baked->nodes)
        {
            ConvertedChannel ch;
            ch.nodeIndex = (uint32_t)bn.typed_id;

            for (const ufbx_baked_vec3& k : bn.translation_keys)
                ch.translation.push_back({ (float)(k.time - t0), ToFloat3(k.value) });

            for (const ufbx_baked_quat& k : bn.rotation_keys)
            {
                PackedFloat4 q = ToFloat4(k.value);

                // q and -q are the same rotation, keep consecutive keys in the same hemisphere so interpolation takes the short way
                if (!ch.rotation.empty() && Dot(q, ch.rotation.back().value) < 0.0f)
                    q = { -q.x, -q.y, -q.z, -q.w };

                ch.rotation.push_back({ (float)(k.time - t0), q });
            }

            for (const ufbx_baked_vec3& k : bn.scale_keys)
                ch.scale.push_back({ (float)(k.time - t0), ToFloat3(k.value) });

            if (ch.translation.empty() && ch.rotation.empty() && ch.scale.empty())
                continue;

            anim.channels.push_back(std::move(ch));
        }

        ufbx_free_baked_anim(baked);

        LOG("  Animation \"" << anim.name << "\": " << anim.duration << " s, " << anim.channels.size() << " animated nodes");
        out.push_back(std::move(anim));
    }
}

// ------------------------------------------------------------------------------------------------
// Serialization
// ------------------------------------------------------------------------------------------------

static void WriteModel(const char* output_file, const std::string& name,
    const std::vector<CompiledMaterial>& materials,
    const std::vector<ConvertedSubmodel>& submodels,
    const std::vector<CompiledNode>& nodes,
    const std::vector<ConvertedAnimation>& animations)
{
    BlobWriter blob(sizeof(CompiledModelHeader));

    // Submodels: bulk arrays first, then the table that points at them
    std::vector<CompiledSubmodel> submodel_table;
    for (const ConvertedSubmodel& s : submodels)
    {
        CompiledSubmodel c = {};
        c.nodeIndex     = s.nodeIndex;
        c.materialIndex = s.materialIndex;
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

        submodel_table.push_back(c);
    }

    // Animations: key arrays, then channel tables, then the animation table
    std::vector<CompiledAnimation> animation_table;
    for (const ConvertedAnimation& a : animations)
    {
        std::vector<CompiledChannel> channel_table;
        for (const ConvertedChannel& ch : a.channels)
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

    CompiledModelHeader header = {};
    header.magic            = ValidMagic;
    header.version          = kCurrentVersion;
    CopyName(header.name, sizeof(header.name), name);
    header.materialCount    = (uint32_t)materials.size();
    header.submodelCount    = (uint32_t)submodel_table.size();
    header.nodeCount        = (uint32_t)nodes.size();
    header.animationCount   = (uint32_t)animation_table.size();
    header.materialsOffset  = blob.Append(materials);
    header.submodelsOffset  = blob.Append(submodel_table);
    header.nodesOffset      = blob.Append(nodes);
    header.animationsOffset = blob.Append(animation_table);
    header.fileSize         = blob.Data().size();

    std::memcpy(blob.Data().data(), &header, sizeof(header));

    std::fstream output_stream;
    output_stream.open(output_file, std::fstream::out | std::fstream::binary);
    EXIT_ON_FAIL(output_stream.is_open(), "Could not open output file");
    output_stream.write((const char*)blob.Data().data(), (std::streamsize)blob.Data().size());
    output_stream.close();

    LOG("Wrote: " << (blob.Data().size() / (1024.0f * 1024.0f)) << " MiB");
}

// ------------------------------------------------------------------------------------------------
// Entry point
// ------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    LOGW(L"---------------- Starting RabBit's model converter ----------------");

    auto start_time = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};
#ifdef _WIN32
    localtime_s(&local_time, &time);
#else
    localtime_r(&time, &local_time);
#endif

    LOG("Start time: " << std::put_time(&local_time, "%d-%m-%Y %H:%M:%S"));
    LOG("");

    DEFINE_FIND_LAUNCH_ARG(argc, argv);
    DEFINE_HAS_LAUNCH_ARG(argc, argv);

    const char* input_file = FindLaunchArg("-input");
    EXIT_ON_FAIL(input_file != nullptr, "No -input file specified");
    LOG("Input file: " << input_file);

    const char* output_file = FindLaunchArg("-output");
    EXIT_ON_FAIL(output_file != nullptr, "No -output file specified");
    LOG("Output file: " << output_file);

    const bool skip_animations = HasLaunchArg("-noAnims");
    LOG("Export animations: " << !skip_animations);

    const char* anim_rate_arg = FindLaunchArg("-animRate");
    const double anim_rate = anim_rate_arg ? std::atof(anim_rate_arg) : 30.0;
    EXIT_ON_FAIL(anim_rate > 0.0, "Invalid -animRate");
    LOG("Animation sample rate: " << anim_rate << " Hz");

    // Optional: rewrite texture extensions so they point at the compiled textures, e.g. -textureExt .rbtx
    const char* texture_ext_arg = FindLaunchArg("-textureExt");
    const std::string texture_extension = texture_ext_arg ? texture_ext_arg : "";
    LOG("");

    // Load the scene
    ufbx_load_opts opts = {};
    opts.target_axes                 = ufbx_axes_right_handed_y_up;
    opts.space_conversion            = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS; // Good for Blender, use UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY for Maya
    opts.target_unit_meters          = 1.0f;
    opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES;
    opts.inherit_mode_handling       = UFBX_INHERIT_MODE_HANDLING_HELPER_NODES; // So every node inherits its parent's transform the normal way (T * R * S)

    ufbx_error error;
    ufbx_scene* scene = ufbx_load_file(input_file, &opts, &error);
    EXIT_ON_FAIL(scene != nullptr, "Failed to load model with ufbx, error message: " << error.description.data);

    LOG("Loaded scene: " << scene->nodes.count << " nodes, " << scene->meshes.count << " meshes, "
        << scene->materials.count << " materials, " << scene->anim_stacks.count << " animation stacks");

    // Nodes (bind pose hierarchy). ufbx guarantees parents come before their children
    std::vector<CompiledNode> nodes(scene->nodes.count);
    for (const ufbx_node* n : scene->nodes)
    {
        CompiledNode& dst = nodes[n->typed_id];
        CopyName(dst.name, sizeof(dst.name), std::string(n->name.data, n->name.length));
        dst.parent      = n->parent ? (int32_t)n->parent->typed_id : -1;
        dst.translation = ToFloat3(n->local_transform.translation);
        dst.rotation    = ToFloat4(n->local_transform.rotation);
        dst.scale       = ToFloat3(n->local_transform.scale);
    }

    // Animations
    std::vector<ConvertedAnimation> animations;
    if (!skip_animations)
        ConvertAnimations(scene, anim_rate, animations);

    // Materials
    std::vector<CompiledMaterial> materials(scene->materials.count);
    for (const ufbx_material* material : scene->materials)
    {
        CompiledMaterial& dst = materials[material->typed_id];
        CopyName(dst.diffuseTexture, sizeof(dst.diffuseTexture), GetTextureName(material, texture_extension));
    }

    // Meshes
    std::vector<ConvertedSubmodel> submodels;
    for (const ufbx_node* node : scene->nodes)
    {
        const ufbx_mesh* mesh = node->mesh;
        if (mesh == nullptr)
            continue;

        for (size_t part_idx = 0; part_idx < mesh->material_parts.count; part_idx++)
        {
            const ufbx_mesh_part* part = &mesh->material_parts.data[part_idx];
            if (part->num_triangles == 0)
                continue;

            ConvertedSubmodel submodel;
            if (ConvertMeshPart(mesh, part, node, submodel))
                submodels.push_back(std::move(submodel));
        }
    }

    size_t total_vertices = 0, total_indices = 0, skinned = 0;
    for (const ConvertedSubmodel& s : submodels)
    {
        total_vertices += s.positions.size();
        total_indices  += s.indices.size();
        skinned        += (s.flags & kSubmodel_Skinned) ? 1 : 0;
    }

    LOG("Converted " << submodels.size() << " submodels (" << skinned << " skinned), " << total_vertices << " vertices, " << total_indices << " indices");

    ufbx_free_scene(scene);

    // Write the output file
    WriteModel(output_file, GetFilenameWithoutExtension(output_file), materials, submodels, nodes, animations);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> delta = end_time - start_time;

    LOGW(L"");
    LOGW(L"Conversion time: " << delta.count() << " ms");
    LOGW(L"---------------------------------------------------------------------");
    LOGW(L"Succesfully finished writing to the output file");

    return 0;
}

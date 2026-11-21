#include <algorithm>
#include <cfloat>
#include <string>
#include <vector>

#include "FbxImporter.h"
#include "Utils.h"
#include "Log.h"

#include <ufbx.h>

namespace RB::ModelConverter
{
    namespace
    {
        // Used to guess a texture name when the FBX does not reference one
        constexpr const char* kAlbedoSuffix = "_Albedo";
        constexpr const char* kNormalSuffix = "_Normal";

        std::string ToString(const ufbx_string& s)
        {
            return std::string(s.data, s.length);
        }

        PackedFloat3 ToFloat3(const ufbx_vec3& v)
        {
            return { (float)v.x, (float)v.y, (float)v.z };
        }

        PackedFloat4 ToFloat4(const ufbx_quat& q)
        {
            return { (float)q.x, (float)q.y, (float)q.z, (float)q.w };
        }

        void ToFloat4x4(const ufbx_matrix& m, float out[16])
        {
            // Transpose ufbx's column-vector matrix into the engine's row-vector matrix convention.
            out[0]  = (float)m.m00; out[1]  = (float)m.m10; out[2]  = (float)m.m20; out[3]  = 0.0f;
            out[4]  = (float)m.m01; out[5]  = (float)m.m11; out[6]  = (float)m.m21; out[7]  = 0.0f;
            out[8]  = (float)m.m02; out[9]  = (float)m.m12; out[10] = (float)m.m22; out[11] = 0.0f;
            out[12] = (float)m.m03; out[13] = (float)m.m13; out[14] = (float)m.m23; out[15] = 1.0f;
        }

        float Dot(const PackedFloat4& a, const PackedFloat4& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        }

        std::string GetTextureName(const ufbx_texture* tex, const ufbx_material* material, const char* fallback_suffix)
        {
            std::string name;
            if (tex)
                name = ToString(tex->filename);
            else
                name = ToString(material->name) + fallback_suffix + ".png";

            // Strip the directory, the engine finds textures by file name
            const size_t slash = name.find_last_of("/\\");
            if (slash != std::string::npos)
                name = name.substr(slash + 1);

            return name;
        }

        BlendMode GetBlendMode(const ufbx_material* material)
        {
            // ufbx derives the PBR opacity from the FBX transparency properties as well
            const ufbx_material_map& opacity = material->pbr.opacity;
            if (opacity.has_value && opacity.value_real < 1.0)
                return BlendMode::Blended;

            return BlendMode::Opaque;
        }

        CompiledSkinVertex MakeSkinVertex(const ufbx_skin_deformer* skin, uint32_t position_vertex_index)
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

        bool ConvertMeshPart(const ufbx_mesh* mesh, const ufbx_mesh_part* part, const ufbx_node* node, ImportedSubmesh& out)
        {
            out = ImportedSubmesh{};
            out.nodeIndex = (uint32_t)node->typed_id;

            // Materials are imported in typed_id order, so the typed_id is the index into ImportedScene::materials
            if (part->index < mesh->materials.count && mesh->materials.data[part->index] != nullptr)
                out.materialIndex = (uint32_t)mesh->materials.data[part->index]->typed_id;

            const ufbx_skin_deformer* skin = mesh->skin_deformers.count > 0 ? mesh->skin_deformers.data[0] : nullptr;
            if (skin && skin->clusters.count == 0)
                skin = nullptr;

            // The vertices stay in the local space of the node, the node's transform (and its parents') is applied at runtime through the node hierarchy
            if (skin)
            {
                out.flags |= kSubmesh_Skinned;

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

                    const PackedFloat3 p = ToFloat3(pos);

                    positions[vi_global] = p;
                    vertices[vi_global].normal = ToFloat3(normal);
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

        void ConvertAnimations(const ufbx_scene* scene, double sample_rate, std::vector<ImportedAnimation>& out)
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

                ImportedAnimation anim;
                anim.name     = ToString(stack->name);
                anim.duration = (float)baked->playback_duration;

                const double t0 = baked->playback_time_begin;

                for (const ufbx_baked_node& bn : baked->nodes)
                {
                    ImportedChannel ch;
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
    }

    bool FbxImporter::Import(const std::filesystem::path& file, const ImportOptions& options, ImportedScene& out)
    {
        ufbx_load_opts opts = {};
        opts.target_axes                 = ufbx_axes_right_handed_y_up;
        opts.space_conversion            = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS; // Good for Blender, use UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY for Maya
        opts.target_unit_meters          = 1.0f;
        opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_HELPER_NODES;
        opts.inherit_mode_handling       = UFBX_INHERIT_MODE_HANDLING_HELPER_NODES; // So every node inherits its parent's transform the normal way (T * R * S)

        ufbx_error error;
        ufbx_scene* scene = ufbx_load_file(file.string().c_str(), &opts, &error);
        if (scene == nullptr)
        {
            LOG("Failed to load FBX with ufbx, error message: " << error.description.data);
            return false;
        }

        LOG("Loaded scene: " << scene->nodes.count << " nodes, " << scene->meshes.count << " meshes, "
            << scene->materials.count << " materials, " << scene->anim_stacks.count << " animation stacks");

        // Nodes (bind pose hierarchy). ufbx guarantees parents come before their children
        out.nodes.resize(scene->nodes.count);
        for (const ufbx_node* n : scene->nodes)
        {
            CompiledNode& dst = out.nodes[n->typed_id];
            CopyName(dst.name, sizeof(dst.name), ToString(n->name));
            dst.parent      = n->parent ? (int32_t)n->parent->typed_id : -1;
            dst.translation = ToFloat3(n->local_transform.translation);
            dst.rotation    = ToFloat4(n->local_transform.rotation);
            dst.scale       = ToFloat3(n->local_transform.scale);
        }

        // Animations
        if (options.importAnimations)
            ConvertAnimations(scene, options.animationSampleRate, out.animations);

        // Materials, indexed by typed_id
        out.materials.resize(scene->materials.count);
        for (const ufbx_material* material : scene->materials)
        {
            ImportedMaterial& dst = out.materials[material->typed_id];
            dst.name           = ToString(material->name);
            dst.blendMode      = GetBlendMode(material);
            dst.diffuseTexture = GetTextureName(material->fbx.diffuse_color.texture, material, kAlbedoSuffix);
            dst.normalTexture  = GetTextureName(material->fbx.normal_map.texture, material, kNormalSuffix);
        }

        // Meshes
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

                ImportedSubmesh submesh;
                if (ConvertMeshPart(mesh, part, node, submesh))
                    out.submeshes.push_back(std::move(submesh));
            }
        }

        ufbx_free_scene(scene);
        return true;
    }
}

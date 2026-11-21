#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <set>
#include <string>
#include <vector>

#include "Log.h"
#include "ArgParsing.h"
#include "importers/Importer.h"
#include "writers/Writers.h"

using namespace RB::ModelConverter;
namespace fs = std::filesystem;

// ------------------------------------------------------------------------------------------------
// Material naming. Material names become file names, so they need to be valid, unique and fit in the name field
// ------------------------------------------------------------------------------------------------

static std::string SanitizeName(const std::string& name)
{
    std::string out = name;
    for (char& c : out)
    {
        if (!std::isalnum((unsigned char)c) && c != '_' && c != '-')
            c = '_';
    }
    return out;
}

static std::string ToLower(std::string s)
{
    for (char& c : s)
        c = (char)std::tolower((unsigned char)c);
    return s;
}

static std::vector<std::string> MakeMaterialNames(const std::vector<ImportedMaterial>& materials, const std::string& model_name)
{
    constexpr size_t kMaxLength = kMaxMaterialNameLength - 1; // Room for the null terminator

    std::vector<std::string> names;
    std::set<std::string> used; // Lower case, file names are case insensitive on Windows

    for (size_t i = 0; i < materials.size(); i++)
    {
        std::string base = SanitizeName(materials[i].name);
        if (base.empty())
            base = SanitizeName(model_name) + "_material" + std::to_string(i);

        if (base.size() > kMaxLength)
        {
            LOG("Warning: material name \"" << base << "\" is longer than " << kMaxLength << " characters and will be truncated");
            base.resize(kMaxLength);
        }

        std::string name = base;
        for (int suffix = 2; used.count(ToLower(name)) != 0; suffix++)
        {
            const std::string postfix = "_" + std::to_string(suffix);
            name = base.substr(0, std::min(base.size(), kMaxLength - postfix.size())) + postfix;
        }

        if (name != base)
            LOG("Warning: material name \"" << base << "\" is already used, renamed to \"" << name << "\"");

        used.insert(ToLower(name));
        names.push_back(name);
    }

    return names;
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

    const char* input_arg = FindLaunchArg("-input");
    EXIT_ON_FAIL(input_arg != nullptr, "No -input file specified");
    const fs::path input_file = input_arg;
    LOG("Input file: " << input_file.string());

    const char* output_arg = FindLaunchArg("-output");
    EXIT_ON_FAIL(output_arg != nullptr, "No -output directory specified");
    const fs::path output_dir = output_arg;
    LOG("Output directory: " << output_dir.string());

    ImportOptions options;
    options.importAnimations = !HasLaunchArg("-noAnims");
    LOG("Export animations: " << options.importAnimations);

    const char* anim_rate_arg = FindLaunchArg("-animRate");
    options.animationSampleRate = anim_rate_arg ? std::atof(anim_rate_arg) : 30.0;
    EXIT_ON_FAIL(options.animationSampleRate > 0.0, "Invalid -animRate");
    LOG("Animation sample rate: " << options.animationSampleRate << " Hz");

    // Optional: rewrite texture extensions so they point at the compiled textures, e.g. -textureExt .rbtx
    const char* texture_ext_arg = FindLaunchArg("-textureExt");
    const std::string texture_extension = texture_ext_arg ? texture_ext_arg : "";
    LOG("Texture extension: " << (texture_ext_arg ? texture_ext_arg : "unchanged"));

    LOG("");

    // Import
    std::unique_ptr<Importer> importer = CreateImporter(input_file);
    EXIT_ON_FAIL(importer != nullptr, "Unsupported input format: " << input_file.extension().string().c_str());

    ImportedScene scene;
    EXIT_ON_FAIL(importer->Import(input_file, options, scene), "Failed to import the input file");

    size_t total_vertices = 0, total_indices = 0, skinned = 0;
    for (const ImportedSubmesh& s : scene.submeshes)
    {
        total_vertices += s.positions.size();
        total_indices  += s.indices.size();
        skinned        += (s.flags & kSubmesh_Skinned) ? 1 : 0;
    }
    LOG("Imported " << scene.submeshes.size() << " submeshes (" << skinned << " skinned), " << total_vertices << " vertices, "
        << total_indices << " indices, " << scene.materials.size() << " materials, " << scene.animations.size() << " animations");
    LOG("");

    // Write
    std::error_code ec;
    fs::create_directories(output_dir, ec);
    EXIT_ON_FAIL(!ec, "Could not create the output directory");

    const std::string model_name = input_file.stem().string();
    const std::string mesh_file  = model_name + ".rbms";

    EXIT_ON_FAIL(WriteMesh(output_dir / mesh_file, model_name, scene), "Failed to write the mesh file");

    const std::vector<std::string> material_names = MakeMaterialNames(scene.materials, model_name);
    std::vector<std::string> material_files;
    for (size_t i = 0; i < scene.materials.size(); i++)
    {
        material_files.push_back(material_names[i] + ".rbmt");
        EXIT_ON_FAIL(WriteMaterial(output_dir / material_files.back(), material_names[i], scene.materials[i], texture_extension), "Failed to write a material file");
    }

    EXIT_ON_FAIL(WriteModel(output_dir / (model_name + ".rbmd"), model_name, mesh_file, material_files, scene), "Failed to write the model file");

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> delta = end_time - start_time;

    LOGW(L"");
    LOGW(L"Conversion time: " << delta.count() << " ms");
    LOGW(L"---------------------------------------------------------------------");
    LOGW(L"Succesfully finished writing the output files");

    return 0;
}

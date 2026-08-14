#include "Compiler.h"
#include "Log.h"

#include <fstream>
#include <filesystem>
#include <sstream>
#include <regex>

#include <slang-com-helper.h>
#include <slang-cpp-types-core.h>

using namespace RB::ShaderCompiler;
using namespace slang;

#define DEBUG_PRINT 0

Compiler::Compiler()
{
    SlangGlobalSessionDesc desc = {};
    createGlobalSession(&desc, m_GlobalSession.writeRef());
}

void Compiler::FindFiles(std::vector<std::string> source_dirs, std::vector<std::string>& dirs, std::vector<std::filesystem::path>& files)
{
    for (const auto& base_path : source_dirs)
    {
        dirs.push_back(base_path);

        for (const auto& entry : std::filesystem::directory_iterator(base_path))
        {
            if (entry.is_directory())
            {
                std::vector<std::string> new_path = { entry.path().string() };
                FindFiles(new_path, dirs, files);
            }
            else
            {
                if (std::wstring(entry.path().c_str()).find(L".slang") != std::wstring::npos)
                {
                    files.push_back(entry.path());
                }
            }
        }
    }
}

void Compiler::CompileFiles(std::vector<std::string> source_dirs)
{
    TargetDesc session_target = {};
#if RB_SHADER_COMPILER_D3D12
    session_target.format  = SLANG_DXIL;
    session_target.profile = m_GlobalSession->findProfile("sm_6_6"); //sm_6_9
#elif RB_SHADER_COMPILER_VK
    session_target.format = SLANG_SPIRV;
    session_target.profile = m_GlobalSession->findProfile("spirv_1_6");
#endif
    
    std::vector<std::string> dirs;
    std::vector<std::filesystem::path> files;
    FindFiles(source_dirs, dirs, files);

    std::vector<const char*> search_paths;
    for (const auto& path : dirs)
    {
        search_paths.push_back(path.c_str());
    }
    
    PreprocessorMacroDesc global_macros[] = 
    { 
        { "SHADER", "1" },
#if RB_SHADER_COMPILER_D3D12
        { "SHADER_DX12", "1" },
#elif RB_SHADER_COMPILER_VK
        { "SHADER_VK", "1" },
#endif
    };
    
    SessionDesc session_desc = {};
    session_desc.targets                 = &session_target;
    session_desc.targetCount             = 1;
    session_desc.searchPaths             = search_paths.data();
    session_desc.searchPathCount         = search_paths.size();
    session_desc.preprocessorMacros      = global_macros;
    session_desc.preprocessorMacroCount  = _countof(global_macros);
    session_desc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR; // HLSL standard
    
    Slang::ComPtr<ISession> session;
    m_GlobalSession->createSession(session_desc, session.writeRef());

    for (const std::wstring& file : files)
    {
        auto path = std::filesystem::path(file);

        std::ifstream t(path);
        std::stringstream buffer;
        buffer << t.rdbuf();

        Slang::ComPtr<IModule> module;
        Slang::ComPtr<IBlob> diag_blob;
        module = session->loadModule(path.filename().stem().string().c_str(), diag_blob.writeRef());

        EXIT_ON_FAIL(module, L"Failed to load module: " << file.c_str() << "\n" << (const char*)diag_blob->getBufferPointer());

        LOGW(L"Succesfully loaded module: \"" << file.c_str() << "\"");

        std::vector<Slang::ComPtr<IComponentType>> components_to_link;
        for (int entry_point_idx = 0; entry_point_idx < module->getDefinedEntryPointCount(); entry_point_idx++)
        {
            Slang::ComPtr<IEntryPoint> entry;
            EXIT_ON_FAIL_SL(module->getDefinedEntryPoint(entry_point_idx, entry.writeRef()), L"Could not get entry point");
            components_to_link.push_back(Slang::ComPtr<IComponentType>(entry));
        }

        Slang::ComPtr<IComponentType> composed;
        EXIT_ON_FAIL_SL(session->createCompositeComponentType((IComponentType**)components_to_link.data(), components_to_link.size(), composed.writeRef(), diag_blob.writeRef()), 
            L"Failed to create composite component type: " << (const char*)diag_blob->getBufferPointer());

        CompilerOptionEntry options[] = 
        {
            { CompilerOptionName::WarningsAsErrors,         CompilerOptionValue{.kind = CompilerOptionValueKind::String,    .stringValue0   = "all"                             }},
            { CompilerOptionName::PreserveParameters,       CompilerOptionValue{.kind = CompilerOptionValueKind::Int,       .intValue0      = true                              }},
            //{ CompilerOptionName::ForceDXLayout,            CompilerOptionValue{.kind = CompilerOptionValueKind::Int,       .intValue0      = true                              }},
#if RB_CONFIG_DEBUG
            { CompilerOptionName::DebugInformation,         CompilerOptionValue{.kind = CompilerOptionValueKind::Int,       .intValue0      = SLANG_DEBUG_INFO_LEVEL_MAXIMAL    }},
            { CompilerOptionName::DebugInformationFormat,   CompilerOptionValue{.kind = CompilerOptionValueKind::Int,       .intValue0      = SLANG_DEBUG_INFO_FORMAT_PDB       }},
            { CompilerOptionName::Optimization,             CompilerOptionValue{.kind = CompilerOptionValueKind::Int,       .intValue0      = SLANG_OPTIMIZATION_LEVEL_NONE     }},
    #if RB_SHADER_COMPILER_VK
            //VulkanBindShift
            //VulkanBindGlobals
            { CompilerOptionName::VulkanEmitReflection,     CompilerOptionValue{.kind = CompilerOptionValueKind::Int,       .intValue0      = true                              }},
    #endif
#else
            { CompilerOptionName::DebugInformation,         CompilerOptionValue{.kind = CompilerOptionValueKind::Int,       .intValue0      = SLANG_DEBUG_INFO_LEVEL_NONE       }},
            { CompilerOptionName::Optimization,             CompilerOptionValue{.kind = CompilerOptionValueKind::Int,       .intValue0      = SLANG_OPTIMIZATION_LEVEL_HIGH     }},
#endif
        };

        Slang::ComPtr<IComponentType> program;
        EXIT_ON_FAIL_SL(composed->linkWithOptions(program.writeRef(), _countof(options), options, diag_blob.writeRef()), L"Failed o link program: " << (const char*)diag_blob->getBufferPointer());

        ProgramLayout* program_layout = program->getLayout(0, diag_blob.writeRef());
        EXIT_ON_FAIL(program_layout, L"Failed to get program layout: " << (const char*)diag_blob->getBufferPointer());

        std::vector<GlobalParameter> global_params;
        ReflectGlobalScope(program_layout->getGlobalParamsVarLayout(), &global_params);

        uint32_t entry_params_binding_slot = 0;
        for (int entry_point_idx = 0; entry_point_idx < program_layout->getEntryPointCount(); entry_point_idx++)
        {
            EntryPointReflection* entry_point = program_layout->getEntryPointByIndex(entry_point_idx);
            LOG("Found: " << entry_point->getName());

            RB::ShaderCompiler::ShaderReflection compiled_shader = {};
            compiled_shader.entryName           = entry_point->getName();
            compiled_shader.globalParameters    = global_params;
            
            switch (entry_point->getStage())
            {
            case SLANG_STAGE_VERTEX:    compiled_shader.stage = Stage::kVertex;  break;
            case SLANG_STAGE_PIXEL:     compiled_shader.stage = Stage::kPixel;   break;
            case SLANG_STAGE_COMPUTE:   compiled_shader.stage = Stage::kCompute; break;
            default:
                EXIT_ON_FAIL(false, "Stage type not yet supported");
                break;
            }

            ReflectEntryPointParameters(entry_point, &compiled_shader);
            compiled_shader.entryParametersBindingIndex = compiled_shader.entryPointParameters.empty() ? UINT32_MAX : entry_params_binding_slot++;

            Slang::ComPtr<IBlob> shader_blob;
            program->getEntryPointCode(entry_point_idx, 0, shader_blob.writeRef(), diag_blob.writeRef());
            EXIT_ON_FAIL(shader_blob, L"Failed to get entry point code of: " << entry_point_idx << ", " << (const char*)diag_blob->getBufferPointer());
            m_ShaderBlobs.push_back(shader_blob);

            m_Reflections.push_back(compiled_shader);
        }

        m_ModuleParameters.emplace(module->getName(), global_params);
    }
}

uint32_t GetScalarSize(TypeReflection::ScalarType type)
{
    switch (type)
    {
    case SLANG_SCALAR_TYPE_FLOAT32: return 4;
    case SLANG_SCALAR_TYPE_FLOAT16: return 2;
    case SLANG_SCALAR_TYPE_INT32:   return 4;
    case SLANG_SCALAR_TYPE_INT16:   return 2;
    case SLANG_SCALAR_TYPE_UINT32:  return 4;
    case SLANG_SCALAR_TYPE_UINT16:  return 2;
    default: 
        EXIT_ON_FAIL(false, "SIZE OF SCALAR TYPE NOT YET IMPLEMENTED: " << (int)type);
        return 0;
    }
}

void ReflectEntryFields(VariableLayoutReflection* var, uint32_t base_offset, RB::ShaderCompiler::ShaderReflection* out_shader)
{
    TypeLayoutReflection* type_layout = var->getTypeLayout();

    if (type_layout->getKind() == TypeReflection::Kind::Struct)
    {
        for (uint32_t i = 0; i < type_layout->getFieldCount(); i++)
        {
            auto field = type_layout->getFieldByIndex(i);

            if (field->getCategory() != SLANG_PARAMETER_CATEGORY_UNIFORM)
                continue;

            uint32_t offset = base_offset + field->getOffset(SLANG_PARAMETER_CATEGORY_UNIFORM);

            ReflectEntryFields(field, offset, out_shader);
        }
        return;
    }

#if DEBUG_PRINT
    LOG("       name: " << var->getName());
    LOG("       binding offset: " << base_offset);
    LOG("       size: " << type_layout->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM));
    LOG("       kind: " << (int)type_layout->getKind());
    LOG("");
#endif

    EXIT_ON_FAIL(type_layout->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM) == 8 && type_layout->getKind() == TypeReflection::Kind::Vector, "Entry parameters can only be handles to render resources!");

    EntryParameter entry_param = {};
    entry_param.name            = var->getName();
    entry_param.bindingOffset   = base_offset;
    entry_param.size            = type_layout->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM);
    out_shader->entryPointParameters.push_back(entry_param);
}

void Compiler::ReflectEntryPointParameters(EntryPointReflection* entry, RB::ShaderCompiler::ShaderReflection* out_shader)
{
    if (entry->getParameterCount() == 0)
        return;

#if DEBUG_PRINT
    LOG("   Entry parameters:");
#endif

    for (int i = 0; i < entry->getParameterCount(); i++)
    {
        VariableLayoutReflection* param = entry->getParameterByIndex(i);

        auto type_layout = param->getTypeLayout();

        if (param->getCategory() == SLANG_PARAMETER_CATEGORY_VARYING_INPUT && entry->getStage() == SLANG_STAGE_VERTEX)
        {
#if DEBUG_PRINT
            LOG("       vertex input:");
#endif

            EXIT_ON_FAIL(type_layout->getKind() == TypeReflection::Kind::Struct, L"Only vertex inputs inside structs are supported");

            for (int i = 0; i < type_layout->getFieldCount(); i++)
            {
                auto field = type_layout->getFieldByIndex(i);
                auto field_layout = field->getTypeLayout();

                TypeReflection* field_type = field->getTypeLayout()->getType();

                uint32_t scalar_size = GetScalarSize(field_type->getScalarType());

                uint32_t size = 0;
                uint32_t elements = 0;
                if (field_type->getElementCount() > 0) // vector
                {
                    elements = field_type->getElementCount();
                    size = scalar_size * elements;
                }
                else if (field_type->getRowCount() > 0 && field_type->getColumnCount() > 0) // matrix
                {
                    elements = field_type->getRowCount() * field_type->getColumnCount();
                    size = scalar_size * elements;
                }
                else // scalar
                {
                    elements = 1;
                    size = scalar_size;
                }

#if DEBUG_PRINT
                LOG("           name: " << field->getName());
                LOG("           semantic: " << field->getSemanticName());
                LOG("           semantic index: " << field->getSemanticIndex());
                LOG("           size: " << size);
                LOG("           elems: " << elements);
                LOG("           scalar type: " << field_type->getScalarType());
#endif
                VertexEntryParameter vertex_param = {};
                vertex_param.name           = field->getName();
                vertex_param.semantic       = field->getSemanticName();
                vertex_param.semanticIndex  = field->getSemanticIndex();
                vertex_param.elements       = elements;
                vertex_param.size           = size;

                switch (field_type->getScalarType())
                {
                case SLANG_SCALAR_TYPE_FLOAT32: vertex_param.scalarType = ScalarType::Float32; break;
                case SLANG_SCALAR_TYPE_FLOAT16: vertex_param.scalarType = ScalarType::Float16; break;
                case SLANG_SCALAR_TYPE_UINT32:  vertex_param.scalarType = ScalarType::UInt32;  break;
                case SLANG_SCALAR_TYPE_UINT16:  vertex_param.scalarType = ScalarType::UInt16;  break;
                case SLANG_SCALAR_TYPE_INT32:   vertex_param.scalarType = ScalarType::Int32;   break;
                case SLANG_SCALAR_TYPE_INT16:   vertex_param.scalarType = ScalarType::Int16;   break;
                default:
                    EXIT_ON_FAIL(false, "SCALAR TYPE NOT YET IMPLEMENTED");
                }

                out_shader->vertexParameters.push_back(vertex_param);
            }

            continue;
        }

        // Only support entry parameters inside a struct
        if (type_layout->getKind() != TypeReflection::Kind::Struct)
            continue;

        ReflectEntryFields(param, 0, out_shader);
    }
}

uint32_t GetConstantBufferSize(TypeLayoutReflection* cb_layout)
{
    TypeLayoutReflection* element_layout = cb_layout->getElementTypeLayout();

    if (!element_layout)
        return 0;

#if DEBUG_PRINT
    for (SlangInt i = 0; i < element_layout->getFieldCount(); i++)
    {
        VariableLayoutReflection* field = element_layout->getFieldByIndex(i);
    
        if (field->getCategory() != SLANG_PARAMETER_CATEGORY_UNIFORM)
            continue;
    
        LOG("");
        LOG("           name: " << field->getName());
        LOG("           offset: " << field->getOffset(SLANG_PARAMETER_CATEGORY_UNIFORM));
        LOG("           size: " << field->getTypeLayout()->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM));
    }
#endif

    return element_layout->getSize(ParameterCategory::Uniform);
}

void Compiler::ReflectGlobalScope(VariableLayoutReflection* layout, std::vector<GlobalParameter>* out_parameters)
{
    TypeLayoutReflection* type_layout = layout->getTypeLayout();

    switch (type_layout->getKind())
    {
    case TypeReflection::Kind::Struct:
    {
        if (type_layout->getFieldCount() <= 0)
        {
            return;
        }

#if DEBUG_PRINT
        LOG("   Global parameters:");
#endif
        int param_count = type_layout->getFieldCount();
        for (int struct_idx = 0; struct_idx < param_count; struct_idx++)
        {
            auto param = type_layout->getFieldByIndex(struct_idx);

#if DEBUG_PRINT
            LOG("       name: " << param->getName());
            LOG("       binding index: " << param->getBindingIndex());
            LOG("       binding space: " << param->getBindingSpace());
#endif
            
            GlobalParameter global_param = {};
            global_param.name           = param->getName();
            global_param.bindingIndex   = param->getBindingIndex();

            TypeLayoutReflection* param_layout = param->getTypeLayout();
            switch (param_layout->getKind())
            {
            case TypeReflection::Kind::ConstantBuffer:
            {
                uint32_t size = GetConstantBufferSize(param_layout);
#if DEBUG_PRINT
                LOG("       type: ConstantBuffer");
                LOG("       size: " << size);
#endif
                global_param.type = ParamType::kConstantBuffer;
                global_param.size = size;
            }
            break;

            case TypeReflection::Kind::SamplerState:
            {
#if DEBUG_PRINT
                LOG("       type: Sampler");
#endif
                global_param.type = ParamType::kSampler;
                global_param.size = 0; // Static sampler doesn't have a size
            }
            break;

            default:
                LOG("DOES NOT SUPPORT THIS TYPE IN A GLOBAL STRUCT YET");
                break;
            }
#if DEBUG_PRINT
            LOG("");
#endif
            out_parameters->push_back(global_param);
        }
    }
    break;

    default:
        LOG("DOES NOT SUPPORT THIS GLOBAL SCOPE TYPE YET");
        break;
    }
}

#pragma once

namespace RB::Graphics
{
    enum class ResourceState
    {
        COMMON                              = 0,
        VERTEX_AND_CONSTANT_BUFFER          = 1,
        INDEX_BUFFER                        = 2,
        RENDER_TARGET                       = 3,
        UNORDERED_ACCESS                    = 4,
        DEPTH_WRITE                         = 5,
        DEPTH_READ                          = 6,
        NON_PIXEL_SHADER_RESOURCE           = 7,
        PIXEL_SHADER_RESOURCE               = 8,
        COPY_DEST                           = 9,
        COPY_SOURCE                         = 10,
        RAYTRACING_ACCELERATION_STRUCTURE   = 11,
        READ                                = 12,
        ALL_SHADER_RESOURCE                 = 13,
        PRESENT                             = 14,
    };

    enum class RenderResourceFormat
    {
        Unkown,
        R32G32B32A32_TYPELESS,
        R32G32B32A32_FLOAT,
        R16G16B16A16_FLOAT,
        R32G32_FLOAT,
        R8_UINT,
        R32_UINT,
        R8G8B8A8_TYPELESS,
        R8G8B8A8_UNORM,
        R8G8B8A8_SRGB,
        R11G11B10_FLOAT,
        R16G16_FLOAT,
        R16G16_UINT,
        R16_FLOAT,
        R16_UINT,
        R16_UNORM,
        R16_SNORM,
        R8_UNORM,
        R32_FLOAT
    };

    uint32_t GetElementSizeFromFormat(const RenderResourceFormat& format);

    enum class RenderResourceType : uint32_t
    {
        Unknown             = (0 << 0),

        // Primitive types
        Buffer              = (1 << 0),
        Texture             = (1 << 1),

        kLastPrimitiveType  = Texture,

        // Implementation types
        StructuredBuffer    = (1 << 2) | Buffer,
        VertexBuffer        = (1 << 3) | Buffer,
        IndexBuffer         = (1 << 4) | Buffer,
        Texture2D           = (1 << 5) | Texture
    };

    class RenderResource
    {
    public:
        virtual ~RenderResource() = default;

        virtual const char* GetName() const = 0;

        virtual void* GetNativeResource() const = 0;

        virtual RenderResourceFormat GetFormat() const = 0;

        bool IsStreaming() const { return m_IsStreaming; }
        void SetIsStreaming(bool is_streaming) { m_IsStreaming = is_streaming; }

        RenderResourceType GetType() const { return m_Type; }
        RenderResourceType GetPrimitiveType() const;

    protected:
        RenderResource(RenderResourceType type) : m_Type(type), m_IsStreaming(false) {}

        RenderResourceType	m_Type;
        bool				m_IsStreaming;
    };

    class Buffer : public RenderResource
    {
    public:
        virtual ~Buffer() = default;

    protected:
        Buffer(RenderResourceType type) : RenderResource(type) {}
    };

    class StructuredBuffer : public Buffer
    {

    };

    enum class TopologyType
    {
        TriangleList,
        TriangleStrip
    };

    class VertexBuffer : public Buffer
    {
    public:
        virtual ~VertexBuffer() = default;

        RenderResourceFormat GetFormat() const override { return RenderResourceFormat::Unkown; }

        virtual uint32_t GetVertexElementCount() const = 0;
        virtual TopologyType GetTopologyType() const = 0;

        static VertexBuffer* Create(const char* name, const TopologyType& type, void* data, uint32_t vertex_size, uint64_t data_size);

    protected:
        VertexBuffer() : Buffer(RenderResourceType::VertexBuffer) {}
    };

    class IndexBuffer : public Buffer
    {
    public:
        virtual ~IndexBuffer() = default;

        RenderResourceFormat GetFormat() const override { return RenderResourceFormat::R16_UINT; }

        virtual uint64_t GetIndexCount() const = 0;

        static IndexBuffer* Create(const char* name, uint16_t* data, uint64_t data_size);

    protected:
        IndexBuffer() : Buffer(RenderResourceType::IndexBuffer) {}
    };

    #define MAX_TEXTURE_SUBRESOURCE_COUNT 8

    class Texture : public RenderResource
    {
    public:
        virtual ~Texture() = default;

        virtual bool AllowedRenderTarget() const = 0;
        virtual bool AllowedRandomGpuWrites() const = 0;
        virtual bool AllowedDepthStencil() const = 0;

    protected:
        Texture(RenderResourceType type) : RenderResource(type) {}
    };

    class Texture2D : public Texture
    {
    public:
        virtual ~Texture2D() = default;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        float	         GetAspectRatio() const;

        static Texture2D* Create(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool is_depth_stencil, bool random_write_access);
        static Texture2D* Create(const char* name, void* data, uint64_t data_size, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool is_depth_stencil, bool random_write_access);
        static Texture2D* Create(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool is_depth_stencil, bool random_write_access);

    protected:
        Texture2D() : Texture(RenderResourceType::Texture2D) {}
    };

    struct RenderTargetBundle
    {
        Texture2D* colorTargets[8];
        uint32_t   colorTargetsCount;
        Texture2D* depthStencilTarget;
    };
}
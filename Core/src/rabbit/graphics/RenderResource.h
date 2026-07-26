#pragma once

namespace RB::Graphics
{
    enum class ResourceState
    {
        UNKNOWN                             = 0,
        COMMON                              = 1,
        VERTEX_AND_CONSTANT_BUFFER          = 2,
        INDEX_BUFFER                        = 3,
        RENDER_TARGET                       = 4,
        UNORDERED_ACCESS                    = 5,
        DEPTH_WRITE                         = 6,
        DEPTH_READ                          = 7,
        NON_PIXEL_SHADER_RESOURCE           = 8,
        PIXEL_SHADER_RESOURCE               = 9,
        COPY_DEST                           = 10,
        COPY_SOURCE                         = 11,
        RAYTRACING_ACCELERATION_STRUCTURE   = 12,
        READ                                = 13,
        ALL_SHADER_RESOURCE                 = 14,
        PRESENT                             = 15,
    };

    enum class RenderResourceFormat
    {
        Unkown,

        // Regular format
        RGBA32_FLOAT,
        RGBA16_FLOAT,
        RG32_FLOAT,
        R8_UINT,
        R32_UINT,
        RGBA8_UNORM,
        BGRA8_UNORM,
        RGBA8_SRGB,
        RG8_UNORM,
        RG16_FLOAT,
        RG16_UINT,
        R16_FLOAT,
        R16_UINT,
        R16_UNORM,
        R16_SNORM,
        R8_UNORM,
        R32_FLOAT,

        // Depth formats
        D32_FLOAT,
        D16_UNORM,

        // Typeless formats (for example for both depth & regular read/write)
        R32_TYPELESS,

        // Compressed formats
        BC1_UNORM,
        BC1_SRGB,
        BC3_UNORM,
        BC3_SRGB,
        BC4_UNORM,
        BC5_UNORM
    };

    uint32_t GetElementSizeFromFormat(const RenderResourceFormat& format);
    uint32_t GetBytesPerBlockFromFormat(const RenderResourceFormat& format);
    bool IsDepthFormat(const RenderResourceFormat& format);
    bool IsSRGBFormat(const RenderResourceFormat& format);
    bool IsTypelessFormat(const RenderResourceFormat& format);
    bool IsBlockCompressedFormat(const RenderResourceFormat& format);

    enum class RenderResourceType : uint32_t
    {
        Unknown             = (0 << 0),

        // Primitive types
        Buffer              = (1 << 0),
        Texture             = (1 << 1),

        kLastPrimitiveType  = Texture,

        // Implementation types
        GenericBuffer       = (1 << 2) | Buffer,
        VertexBuffer        = (1 << 3) | Buffer,
        IndexBuffer         = (1 << 4) | Buffer,
        ReadbackBuffer      = (1 << 5) | Buffer,
        Texture2D           = (1 << 6) | Texture,
        Texture2DArray      = (1 << 7) | Texture,
        Texture3D           = (1 << 8) | Texture
    };

    class RenderResource
    {
    public:
        virtual ~RenderResource();

        virtual const char* GetName() const { return m_Name.c_str(); }

        virtual void* GetNativeResource() const = 0;

        virtual RenderResourceFormat GetFormat() const = 0;

        virtual bool AllowedRandomReadWrites() const = 0;

        bool ReadyToRender(bool block_until_ready = false);

        RenderResourceType GetType() const { return m_Type; }
        RenderResourceType GetPrimitiveType() const;

    protected:
        RenderResource(const char* name, RenderResourceType type) 
            : m_Name(name)
            , m_Type(type)
            , m_IsStreaming(nullptr)
        {}

        std::string         m_Name;
        RenderResourceType	m_Type;
    
    private:
        void SetStreaming(bool is_streaming);

        ThreadedVariable<bool>* m_IsStreaming;

        friend class ResourceStreamer;
    };

    // -----------------------------------------------------------------------------------------------
    //							                    Buffers
    // -----------------------------------------------------------------------------------------------

    class Buffer : public RenderResource
    {
    public:
        virtual ~Buffer() = default;

        virtual uint64_t GetSize() const = 0;

    protected:
        Buffer(const char* name, RenderResourceType type) : RenderResource(name, type) {}
    };

    class GenericBuffer : public Buffer
    {
    public:
        static Shared<GenericBuffer> Create(const char* name, RenderResourceFormat format, uint32_t elements, bool random_read_write_access);
        static Shared<GenericBuffer> Create(const char* name, uint32_t element_size, uint32_t elements, bool random_read_write_access);

    protected:
        GenericBuffer(const char* name) : Buffer(name, RenderResourceType::GenericBuffer) {}
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

        bool AllowedRandomReadWrites() const override { return false; };

        uint64_t GetSize() const override { return GetVertexElementCount() * GetVertexSize(); }

        virtual uint32_t GetVertexSize() const = 0;
        virtual uint32_t GetVertexElementCount() const = 0;
        virtual TopologyType GetTopologyType() const = 0;

        static Shared<VertexBuffer> Create(const char* name, const TopologyType& type, const void* data, uint32_t vertex_size, uint64_t data_size, bool transient = false);

    protected:
        VertexBuffer(const char* name) : Buffer(name, RenderResourceType::VertexBuffer) {}
    };

    class IndexBuffer : public Buffer
    {
    public:
        virtual ~IndexBuffer() = default;

        RenderResourceFormat GetFormat() const override { return RenderResourceFormat::R32_UINT; }

        bool AllowedRandomReadWrites() const override { return false; };

        uint64_t GetSize() const override { return GetIndexCount() * 4; }

        virtual uint64_t GetIndexCount() const = 0;

        static Shared<IndexBuffer> Create(const char* name, const uint32_t* data, uint64_t elements);

    protected:
        IndexBuffer(const char* name) : Buffer(name, RenderResourceType::IndexBuffer) {}
    };

    class ReadbackBuffer : public Buffer
    {
    public:
        // Make sure that the memory parameter is at least the size of GetPackedSize().
        // If should_block is false it will return older frames' data. You probably want this behaviour when doing readbacks every frame.
        virtual bool GetData(void* memory, bool should_block = false) = 0;

        virtual uint64_t GetPackedSize() const = 0;

        RenderResourceFormat GetFormat() const override { return RenderResourceFormat::Unkown; }

        bool AllowedRandomReadWrites() const override { return false; };

        static Shared<ReadbackBuffer> Create(const char* name, uint64_t size);
        static Shared<ReadbackBuffer> Create(const char* name, RenderResource* target_size);

    protected:
        ReadbackBuffer(const char* name) : Buffer(name, RenderResourceType::ReadbackBuffer) {}
    };

    // -----------------------------------------------------------------------------------------------
    //							                    Textures
    // -----------------------------------------------------------------------------------------------

    uint32_t CalculateMaxMips(uint32_t width, uint32_t height);

    class Texture : public RenderResource
    {
    public:
        virtual ~Texture() = default;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual uint32_t GetDepth() const = 0;
        float	         GetAspectRatio() const;

        virtual uint32_t GetViewportWidth() const = 0;
        virtual uint32_t GetViewportHeight() const = 0;
        virtual uint32_t GetViewportDepth() const = 0;
        float            GetViewportAspectRatio() const;

        virtual void SetViewportWidth(uint32_t width) = 0;
        virtual void SetViewportHeight(uint32_t height) = 0;
        virtual void SetViewportDepth(uint32_t depth) = 0;

        virtual bool AllowedRenderTarget() const = 0;
        virtual bool AllowedDepthStencil() const = 0;

        virtual uint32_t GetMipCount() const = 0;
        virtual uint32_t GetBaseMip() const = 0;

        virtual void SetBaseMip(uint32_t mip) = 0;
        virtual void SetMipCount(uint32_t mips) = 0;

        virtual uint32_t GetArraySize() const = 0;
        virtual uint32_t GetFirstArraySlice() const = 0;

        virtual void SetArraySize(uint32_t size) = 0;
        virtual void SetFirstArraySlice(uint32_t slice) = 0;

        // Resets all the overwritten mip/array properties
        virtual void ResetView() = 0;

    protected:
        Texture(const char* name, RenderResourceType type)
            : RenderResource(name, type)
        {}
    };

    class Texture2D : public Texture
    {
    public:
        virtual ~Texture2D() = default;

        virtual uint32_t GetDepth() const override { return 1; }
        virtual uint32_t GetViewportDepth() const override { return 1; }
        
        virtual void SetViewportDepth(uint32_t depth) override {}

        virtual uint32_t GetArraySize() const override { return 1; }
        virtual uint32_t GetFirstArraySlice() const override { return 0; }

        virtual void SetArraySize(uint32_t size) override {}
        virtual void SetFirstArraySlice(uint32_t slice) override {}

        static Shared<Texture2D> Create(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access);
        static Shared<Texture2D> Create(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t mips, bool is_render_target, bool random_read_write_access);
        static Shared<Texture2D> Create(const char* name, const void* data, uint64_t data_size, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access);
        static Shared<Texture2D> Create(const char* name, const void* data, uint64_t data_size, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t mips, bool is_render_target, bool random_read_write_access);
        static Shared<Texture2D> Create(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height, bool is_render_target, bool random_read_write_access);
        // Copies the view on the resource but will not own the underlying resource
        static Shared<Texture2D> Alias(const Shared<Texture2D>& original);

    protected:
        Texture2D(const char* name) : Texture(name, RenderResourceType::Texture2D) {}
    };

    class Texture2DArray : public Texture
    {
    public:
        virtual ~Texture2DArray() = default;

        virtual uint32_t GetDepth() const override { return 1; }
        virtual uint32_t GetViewportDepth() const override { return 1; }

        virtual void SetViewportDepth(uint32_t depth) override {}

        static Shared<Texture2DArray> Create(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t slices, bool is_render_target, bool random_read_write_access);
        static Shared<Texture2DArray> Alias(const Shared<Texture2DArray>& original);

    protected:
        Texture2DArray(const char* name) : Texture(name, RenderResourceType::Texture2DArray) {}
    };

    class Texture3D : public Texture
    {
    public:
        virtual ~Texture3D() = default;

        virtual bool AllowedDepthStencil() const override { return false; }

        virtual uint32_t GetArraySize() const override { return 1; }
        virtual uint32_t GetFirstArraySlice() const override { return 0; }

        virtual void SetArraySize(uint32_t size) override {}
        virtual void SetFirstArraySlice(uint32_t slice) override {}

        static Shared<Texture3D> Create(const char* name, RenderResourceFormat format, uint32_t width, uint32_t height, uint32_t depth, bool is_render_target, bool random_read_write_access);

    protected:
        Texture3D(const char* name) : Texture(name, RenderResourceType::Texture3D) {}
    };

    struct RenderTargetBundle
    {
        Texture2D* colorTargets[8];
        uint32_t   colorTargetsCount;
        Texture2D* depthStencilTarget;
    };
}
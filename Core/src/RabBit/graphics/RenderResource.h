#pragma once

namespace RB::Graphics
{
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

	enum class RenderResourceType : uint32_t
	{
		Unknown					= (0 << 0),

		// Primitive types
		Buffer					= (1 << 0),
		Texture					= (1 << 1),

		kLastPrimitiveType		= Texture,

		// Implementation types
		StructuredBuffer		= (1 << 2) | Buffer,
		VertexBuffer			= (1 << 3) | Buffer,
		IndexBuffer				= (1 << 4) | Buffer,
		Texture2D				= (1 << 5) | Texture
	};

	class RenderResource
	{
	public:
		virtual ~RenderResource() = default;

		virtual const char* GetName() const = 0;

		virtual void* GetNativeResource() const = 0;

		virtual void* GetData() const = 0;
		virtual uint32_t GetSize() const = 0;
		//virtual void WriteData(void* data, uint64_t size, uint32_t write_offset) = 0;

		//virtual bool IsInFlight() const = 0;
		//virtual bool MarkedForDelete() const = 0;
		//virtual bool AllowedCpuReads() const = 0;
		//virtual bool AllowedCpuWrites() const = 0;

		virtual RenderResourceFormat GetFormat() const = 0;

		RenderResourceType GetType() const { return m_Type; }
		RenderResourceType GetPrimitiveType() const;

	protected:
		RenderResource(RenderResourceType type): m_Type(type) {}

		RenderResourceType m_Type;
	};

	class Buffer : public RenderResource
	{
	public:
		virtual ~Buffer() = default;

	protected:
		Buffer(RenderResourceType type): RenderResource(type) {}
	};

	class StructuredBuffer : public Buffer
	{

	};

	enum class TopologyType
	{
		TriangleList
	};

	class VertexBuffer : public Buffer
	{
	public:
		virtual ~VertexBuffer() = default;
		
		RenderResourceFormat GetFormat() const override { return RenderResourceFormat::Unkown; }

		virtual TopologyType GetTopologyType() const = 0;

		static VertexBuffer* Create(const char* name, const TopologyType& type, void* data, uint64_t data_size);

	protected:
		VertexBuffer(): Buffer(RenderResourceType::VertexBuffer) {}
	};

	class IndexBuffer : public Buffer
	{

	};

	class Texture : public RenderResource
	{
	public:
		virtual ~Texture() = default;

		virtual bool AllowedRenderTarget() const = 0;
		//virtual bool AllowedDepthStencil() = 0;
		//virtual bool AllowedRandomGpuWrites() = 0;
		//virtual bool AllowedGpuReads() = 0;

	protected:
		Texture(RenderResourceType type): RenderResource(type) {}
	};

	class Texture2D : public Texture
	{
	public:
		virtual ~Texture2D() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		static Texture2D* Create(const char* name, void* internal_resource, RenderResourceFormat format, uint32_t width, uint32_t height);

	protected:
		Texture2D(): Texture(RenderResourceType::Texture2D) {}
	};

	class RenderTargetBundle
	{
	public:
		// 0 - 8
		virtual Texture GetColorTarget(uint32_t index) = 0;
		virtual Texture GetDepthStencilTarget() = 0;
	};
}
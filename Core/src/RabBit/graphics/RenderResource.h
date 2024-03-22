#pragma once

namespace RB::Graphics
{
	// TODO Create D3D12 implementation classes of these

	class RenderResource
	{
	public:
		virtual ~RenderResource() = default;

		virtual const char* GetName() const = 0;

		virtual void* GetNativeResource() const = 0;

		virtual void* GetData() = 0;
		virtual void WriteData(void* data, uint64_t size, uint32_t write_offset) = 0;

		virtual bool IsInFlight() const = 0;
		virtual bool MarkedForDelete() const = 0;
		virtual bool AllowedCpuReads() const = 0;
		virtual bool AllowedCpuWrites() const = 0;

	protected:
		RenderResource() = default;
	};

	class Buffer : public RenderResource
	{
	public:
		virtual ~Buffer() = default;

		virtual uint32_t GetWidth() const = 0;

	protected:
		Buffer() = default;
	};

	class StructuredBuffer : public Buffer
	{

	};

	class VertexBuffer : public Buffer
	{
	public:
		virtual ~VertexBuffer() = default;
		
		static VertexBuffer* Create(const char* name, void* data, uint64_t data_size);

	protected:
		VertexBuffer() = default;
	};

	class IndexBuffer : public Buffer
	{

	};

	class Texture : public RenderResource
	{
	public:
		virtual bool IsDepthStencil() = 0;
		virtual bool IsRenderTargetCompatible() = 0;
		virtual bool AllowedRandomGpuWrites() = 0;
		virtual bool AllowedGpuReads() = 0;
	};

	class Texture2D : public Texture
	{
	public:
		virtual uint32_t GetWidth() = 0;
		virtual uint32_t GetHeight() = 0;
	};

	class RenderTargetBundle
	{
	public:
		// 0 - 8
		virtual Texture GetColorTarget(uint32_t index) = 0;
		virtual Texture GetDepthStencilTarget() = 0;
	};
}
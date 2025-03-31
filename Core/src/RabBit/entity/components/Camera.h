#pragma once
#include "RabBitCommon.h"
#include "entity/ObjectComponent.h"
#include "graphics/RenderResource.h"
#include "graphics/Renderer.h"
#include "math/Vector.h"

namespace RB::Entity
{
    class Camera : public ObjectComponent
    {
    public:
        DEFINE_COMP_TAG("Camera");

        // FOV in degrees
        Camera(float near_plane, float far_plane, float vfov, void* target_window_handle)
            : m_Near(near_plane)
            , m_Far(far_plane)
            , m_VFovDegrees(vfov)
            , m_TargetWindowHandle(target_window_handle)
            , m_RenderTexture(nullptr)
            , m_RenderGraphType(Graphics::kRenderGraphType_Normal)
            , m_ClearColor(1.0f)
        {
        }

        Camera(float near_plane, float far_plane, float vfov, Graphics::Texture2D* render_texture)
            : m_Near(near_plane)
            , m_Far(far_plane)
            , m_VFovDegrees(vfov)
            , m_TargetWindowHandle(nullptr)
            , m_RenderTexture(render_texture)
            , m_RenderGraphType(Graphics::kRenderGraphType_Normal)
            , m_ClearColor(1.0f)
        {
        }

        void SetRenderGraphType(Graphics::RenderGraphType type) { m_RenderGraphType = type; }
        uint32_t GetRenderGraphType() const { return m_RenderGraphType; }

        void* GetTargetWindowHandle()	const { return m_TargetWindowHandle; }
        Graphics::Texture2D* GetRenderTexture() const { return m_RenderTexture; }

        void SetClearColor(const Math::Float4& color) { m_ClearColor = color; }
        Math::Float4 GetClearColor() const { return m_ClearColor; }

        float GetNearPlane()			const { return m_Near; }
        float GetFarPlane()				const { return m_Far; }
        float GetVerticalFovInDegrees()	const { return m_VFovDegrees; }
        float GetVerticalFovInRadians()	const { return Math::DegreesToRadians(m_VFovDegrees); }

    private:
        float					    m_Near;
        float					    m_Far;
        float					    m_VFovDegrees;

        Math::Float4			    m_ClearColor;
        void*                       m_TargetWindowHandle;
        Graphics::Texture2D*        m_RenderTexture;
        Graphics::RenderGraphType   m_RenderGraphType;
    };
}
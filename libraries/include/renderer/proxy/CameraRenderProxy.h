#pragma once

#include "core/RenderProxy.h"

#include "rhi/RHIBuffer.h"

namespace sparkle
{
// the camera render proxy is used for:
// 1. translate camera parameters to view info.
// 2. view info management.
// TODO(tqjxlm) separate view from camera
class CameraRenderProxy : public RenderProxy
{
public:
    // chy
    enum class ProjectionType : uint8_t
    {
        Perspective,
        Orthographic,
    };

    static constexpr Scalar OutputLimit = 6.f;

    struct UniformBufferData
    {
        alignas(16) Vector3 position;
        uint32_t mode;
        alignas(16) Vector3 lowerLeft;
        uint32_t max_bounce;
        alignas(16) Vector3 max_u;
        alignas(16) Vector3 max_v;
        float lensRadius;
        uint32_t projection_type; // chy:0=Perspective, 1=Orthographic
        // chy: uint32_t[2] instead of Vector2Int — Eigen over-aligns to 16B, SPIR-V std140 expects 8B alignment
        alignas(8) uint32_t resolution[2];
    };

    // values calculated from physical attributes and used for rendering
    struct Attribute
    {
        float vertical_fov;
        float focus_distance;
        float exposure;
        float aperture_radius;

        // chy
        ProjectionType projection_type = ProjectionType::Perspective;
        float ortho_width = 0.f;

        void Print() const;
    };

    struct Posture
    {
        Vector3 position;
        Vector3 up;
        Vector3 front;
        Vector3 right;
    };

    // focus plane in world space
    struct FocusPlane
    {
        float height;
        float width;
        Vector3 max_u;
        Vector3 max_v;
        Vector3 lower_left;
    };

    void Update(RHIContext *rhi, const CameraRenderProxy &camera, const RenderConfig &config) override;

    void InitRenderResources(RHIContext *rhi, const RenderConfig &config) override;

    void OnTransformDirty(RHIContext *rhi) override;

    void ClearPixels();

    void UpdateAttribute(const Attribute &attribute)
    {
        state_ = attribute;
        attribute_dirty_ = true;
    }

    void MarkPixelDirty()
    {
        pixels_dirty_ = true;
        cumulated_sample_count_ = 0;
        pending_sample_count_ = 0;
    }

    void AccumulateSample(uint32_t sample_count)
    {
        pending_sample_count_ += sample_count;
    }

    [[nodiscard]] RHIResourceRef<RHIBuffer> GetViewBuffer() const
    {
        return view_buffer_;
    }

    [[nodiscard]] bool NeedClear() const
    {
        return pixels_dirty_;
    }

    [[nodiscard]] auto GetAttribute() const
    {
        return state_;
    }

    [[nodiscard]] auto GetPosture() const
    {
        return posture_;
    }

    [[nodiscard]] auto GetFocusPlane() const
    {
        return focus_plane_;
    }

    [[nodiscard]] uint32_t GetCumulatedSampleCount() const
    {
        return cumulated_sample_count_;
    }

    [[nodiscard]] TransformMatrix GetViewMatrix() const
    {
        return view_matrix_;
    }

    [[nodiscard]] Mat4 GetProjectionMatrix() const
    {
        return projection_matrix_;
    }

    [[nodiscard]] Mat4 GetViewProjectionMatrix() const
    {
        return view_projection_matrix_;
    }

    [[nodiscard]] float GetNear() const
    {
        return near_;
    }

    [[nodiscard]] float GetFar() const
    {
        return far_;
    }

    // chy
    [[nodiscard]] UniformBufferData GetUniformBufferData(const RenderConfig &config) const
    {
        return {.position = posture_.position,
                .mode = static_cast<uint32_t>(config.debug_mode),
                .lowerLeft = focus_plane_.lower_left,
                .max_bounce = config.max_bounce,
                .max_u = focus_plane_.max_u,
                .max_v = focus_plane_.max_v,
                .lensRadius = state_.aperture_radius,
                .projection_type = static_cast<uint32_t>(state_.projection_type),  // chy
                .resolution = {static_cast<uint32_t>(config.image_width),
                                static_cast<uint32_t>(config.image_height)}};
    }

private:
    void SetupProjectionMatrix();

    Attribute state_;
    Posture posture_;
    FocusPlane focus_plane_;

    TransformMatrix view_matrix_;
    Mat4 projection_matrix_;
    Mat4 view_projection_matrix_;

    float aspect_ratio_;
    float near_ = 0.1f;
    float far_ = 1000.f;

    unsigned pending_sample_count_ = 0;
    unsigned cumulated_sample_count_ = 0;

    RHIResourceRef<RHIBuffer> view_buffer_;

    uint32_t pixels_dirty_ : 1 = 1;
    uint32_t attribute_dirty_ : 1 = 1;
    uint32_t need_cpu_frame_buffer_ : 1 = 0;
};
} // namespace sparkle

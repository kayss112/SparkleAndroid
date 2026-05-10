#pragma once

#include "scene/component/RenderableComponent.h"

namespace sparkle
{
// chy
enum class ProjectionType : uint8_t
{
    Perspective,
    Orthographic,
};

class CameraComponent : public RenderableComponent
{
public:
    // values that have physical meaning (reflects real camera attributes)
    struct Attribute
    {
        // Perspective params
        float focal_length = 0.035f;  // 35mm
        float sensor_height = 0.024f; // full frame
        float aperture = 22.0f;
        float exposure = 1.f;
        float focus_distance = 1.f;

        // chy
        ProjectionType projection_type = ProjectionType::Perspective;
        float ortho_width = 10.0f;  // Orthographic param

        void Print() const;
    };

    explicit CameraComponent(const Attribute &attribute);

    ~CameraComponent() override;

    void UpdateRenderData();

    virtual void PrintPosture() = 0;

#pragma region Attributes

    [[nodiscard]] auto GetAttribute() const
    {
        return attribute_;
    }

    void SetFocusDistance(float focus_distance);

    void SetAperture(float aperture);

    void SetExposure(float exposure);

    // chy
    void SetProjectionType(ProjectionType type);

    // chy
    void SetOrthoWidth(float width);

#pragma endregion

#pragma region Input

    virtual void OnPointerDown()
    {
    }

    virtual void OnPointerUp()
    {
    }

    virtual void OnPointerMove(float, float)
    {
    }

    virtual void OnScroll(float)
    {
    }

#pragma endregion

#pragma region Component interfaces

    void OnAttach() override;

#pragma endregion

protected:
    std::unique_ptr<RenderProxy> CreateRenderProxy() override;

private:
    Attribute attribute_;
};
} // namespace sparkle

#pragma once

#include "scene/component/primitive/MeshPrimitive.h"

namespace sparkle
{
// Biconvex symmetric lens defined by its sphere radius of curvature and half-thickness.
// Constraint: sphere_radius > half_thickness > 0.
// Non-uniform scaling is not supported; use sphere_radius/half_thickness directly.
class LensPrimitive : public MeshPrimitive
{
public:
    explicit LensPrimitive(float sphere_radius = 2.f, float half_thickness = 0.5f);

    void OnTransformChange() override;

protected:
    std::unique_ptr<RenderProxy> CreateRenderProxy() override;

private:
    float base_sphere_radius_;
    float base_half_thickness_;

    float sphere_radius_;
    float half_thickness_;
};
} // namespace sparkle

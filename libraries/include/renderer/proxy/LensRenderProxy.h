#pragma once

#include "renderer/proxy/MeshRenderProxy.h"

#include "core/math/Intersection.h"

namespace sparkle
{
// Biconvex lens: intersection of two spheres of equal radius, symmetric about the local XY plane.
// Sphere centers are at (0, 0, ±offset) in local space, where offset = sphere_radius - half_thickness.
class LensRenderProxy : public MeshRenderProxy
{
public:
    using MeshRenderProxy::MeshRenderProxy;

    bool Intersect(const Ray &ray, IntersectionCandidate &candidate) const override;

    bool IntersectAnyHit(const Ray &ray, IntersectionCandidate &candidate) const override;

    void GetIntersection(const Ray &ray, const IntersectionCandidate &candidate,
                         Intersection &intersection) override;

    void SetLensParams(float sphere_radius, float half_thickness)
    {
        sphere_radius_ = sphere_radius;
        half_thickness_ = half_thickness;
    }

private:
    template <bool AnyHit> bool IntersectInternal(const Ray &ray, IntersectionCandidate &candidate) const;

    float sphere_radius_ = 2.f;
    float half_thickness_ = 0.5f;
};
} // namespace sparkle

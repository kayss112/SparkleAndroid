#pragma once

#include "renderer/proxy/MeshRenderProxy.h"

#include "core/math/Intersection.h"

namespace sparkle
{
class BoxRenderProxy : public MeshRenderProxy
{
public:
    using MeshRenderProxy::MeshRenderProxy;

    bool Intersect(const Ray &ray, IntersectionCandidate &candidate) const override;

    bool IntersectAnyHit(const Ray &ray, IntersectionCandidate &candidate) const override;

    void GetIntersection(const Ray &ray, const IntersectionCandidate &candidate,
                         Intersection &intersection) override;

private:
    template <bool AnyHit> bool IntersectInternal(const Ray &ray, IntersectionCandidate &candidate) const;
};
} // namespace sparkle

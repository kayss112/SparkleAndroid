#include "renderer/proxy/BoxRenderProxy.h"

#include "core/math/Ray.h"
#include "core/math/Types.h"
#include "core/math/Utilities.h"

namespace sparkle
{
// Unit cube in local space spans [-1, 1]^3 (matches Mesh::GetUnitCube)
template <bool AnyHit>
bool BoxRenderProxy::IntersectInternal(const Ray &world_ray, IntersectionCandidate &candidate) const
{
    const auto inv_transform = GetTransform().GetInverse();
    const Ray local_ray = world_ray.TransformedBy(inv_transform);

    const Vector3 &o = local_ray.Origin();
    const Vector3 &d = local_ray.Direction();

    float t_min = -std::numeric_limits<float>::max();
    float t_max =  std::numeric_limits<float>::max();

    for (int i = 0; i < 3; ++i)
    {
        if (std::abs(d[i]) < Eps)
        {
            if (o[i] < -1.f || o[i] > 1.f)
                return false;
            continue;
        }

        const float inv_d = 1.f / d[i];
        float t1 = (-1.f - o[i]) * inv_d;
        float t2 = ( 1.f - o[i]) * inv_d;
        if (t1 > t2)
            std::swap(t1, t2);
        t_min = std::max(t_min, t1);
        t_max = std::min(t_max, t2);
        if (t_min > t_max)
            return false;
    }

    const float t_local = (t_min > Eps) ? t_min : (t_max > Eps ? t_max : -1.f);
    if (t_local < 0.f)
    {
        return false;
    }

    if constexpr (AnyHit){
        return true;
    }


    const Vector3 world_p = GetTransform().TransformPoint(local_ray.At(t_local));
    const float world_t = world_ray.InverseAt(world_p);

    if (world_t > 0.f && candidate.IsCloserHit(world_t))
    {
        candidate.t = world_t;
        return true;
    }
    return false;
}

bool BoxRenderProxy::Intersect(const Ray &ray, IntersectionCandidate &candidate) const
{
    return IntersectInternal<false>(ray, candidate);
}

bool BoxRenderProxy::IntersectAnyHit(const Ray &ray, IntersectionCandidate &candidate) const
{
    return IntersectInternal<true>(ray, candidate);
}

void BoxRenderProxy::GetIntersection(const Ray &ray, const IntersectionCandidate &candidate,
                                     Intersection &intersection)
{
    const auto inv_transform = GetTransform().GetInverse();
    const Vector3 local_p = inv_transform.TransformPoint(ray.At(candidate.t));

    // Find the face whose axis-aligned plane was hit (closest to ±1)
    Vector3 local_normal = Vector3::Zero();
    float min_dist = std::numeric_limits<float>::max();
    for (int i = 0; i < 3; ++i)
    {
        const float dist = std::abs(std::abs(local_p[i]) - 1.f);
        if (dist < min_dist)
        {
            min_dist = dist;
            local_normal = Vector3::Zero();
            local_normal[i] = local_p[i] > 0.f ? 1.f : -1.f;
        }
    }

    // Inverse transpose for normal transform
    const Vector3 world_normal = inv_transform.TransformDirectionTangentSpace(local_normal).normalized();
    const Vector3 world_tangent = utilities::GetPossibleMajorAxis(world_normal);
    intersection.Update(ray, this, candidate.t, world_normal, world_tangent);
}
} // namespace sparkle

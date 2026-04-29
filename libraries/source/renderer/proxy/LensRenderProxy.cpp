#include "renderer/proxy/LensRenderProxy.h"

#include "core/math/Ray.h"
#include "core/math/Types.h"
#include "core/math/Utilities.h"

namespace sparkle
{
// face_idx == 0 → hit on sphere 1 (front, -Z side)
// face_idx == 1 → hit on sphere 2 (back,  +Z side)
template <bool AnyHit>
bool LensRenderProxy::IntersectInternal(const Ray &world_ray, IntersectionCandidate &candidate) const
{
    const auto inv_transform = GetTransform().GetInverse();
    const Ray local_ray = world_ray.TransformedBy(inv_transform);

    const Vector3 &o = local_ray.Origin();
    const Vector3 &d = local_ray.Direction();

    const float offset = sphere_radius_ - half_thickness_;
    const float R2 = sphere_radius_ * sphere_radius_;
    const float a = d.squaredNorm();

    // Returns [t_enter, t_exit] for a sphere; invalid if discriminant < 0
    auto sphere_interval = [&](float z_center) -> std::pair<float, float>
    {
        const Vector3 oc = o - Vector3(0.f, 0.f, z_center);
        const float half_b = d.dot(oc);
        const float disc = half_b * half_b - a * (oc.squaredNorm() - R2);
        if (disc < 0.f)
            return {std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
        const float sq = std::sqrt(disc);
        return {(-half_b - sq) / a, (-half_b + sq) / a};
    };

    auto [s1a, s1b] = sphere_interval(-offset);  // front sphere
    auto [s2a, s2b] = sphere_interval( offset);  // back sphere

    // Lens = intersection of the two sphere interiors
    const float t_enter = std::max(s1a, s2a);
    const float t_exit  = std::min(s1b, s2b);

    if (t_enter >= t_exit)
        return false;

    float t_local;
    uint32_t hit_sphere;

    if (t_enter > Eps)
    {
        t_local = t_enter;
        hit_sphere = (s1a > s2a) ? 0u : 1u;
    }
    else if (t_exit > Eps)
    {
        t_local = t_exit;
        hit_sphere = (s1b < s2b) ? 0u : 1u;
    }
    else
    {
        return false;
    }

    if constexpr (AnyHit)
        return true;

    const Vector3 world_p = GetTransform().TransformPoint(local_ray.At(t_local));
    const float world_t = world_ray.InverseAt(world_p);

    if (world_t > 0.f && candidate.IsCloserHit(world_t))
    {
        candidate.t = world_t;
        candidate.face_idx = hit_sphere;
        return true;
    }
    return false;
}

bool LensRenderProxy::Intersect(const Ray &ray, IntersectionCandidate &candidate) const
{
    return IntersectInternal<false>(ray, candidate);
}

bool LensRenderProxy::IntersectAnyHit(const Ray &ray, IntersectionCandidate &candidate) const
{
    return IntersectInternal<true>(ray, candidate);
}

void LensRenderProxy::GetIntersection(const Ray &ray, const IntersectionCandidate &candidate,
                                      Intersection &intersection)
{
    const float offset = sphere_radius_ - half_thickness_;
    const float z_center = candidate.face_idx == 0 ? -offset : offset;
    const Vector3 c_world = GetTransform().TransformPoint(Vector3(0.f, 0.f, z_center));

    const Vector3 p = ray.At(candidate.t);
    const Vector3 normal = (p - c_world).normalized();
    const Vector3 tangent = utilities::GetPossibleMajorAxis(normal);
    intersection.Update(ray, this, candidate.t, normal, tangent);
}
} // namespace sparkle

#include "scene/component/primitive/LensPrimitive.h"

#include "core/task/TaskManager.h"
#include "io/Mesh.h"
#include "renderer/proxy/LensRenderProxy.h"

namespace sparkle
{
LensPrimitive::LensPrimitive(float sphere_radius, float half_thickness)
    : MeshPrimitive(Mesh::GetUnitSphere()),
      base_sphere_radius_(sphere_radius),
      base_half_thickness_(half_thickness),
      sphere_radius_(sphere_radius),
      half_thickness_(half_thickness)
{
}

std::unique_ptr<RenderProxy> LensPrimitive::CreateRenderProxy()
{
    auto proxy = std::make_unique<LensRenderProxy>(raw_mesh_, node_->GetName(), local_bound_);
    proxy->SetLensParams(sphere_radius_, half_thickness_);
    return proxy;
}

void LensPrimitive::OnTransformChange()
{
    MeshPrimitive::OnTransformChange();

    const float scale = GetLocalTransform().GetScale().maxCoeff();
    sphere_radius_ = base_sphere_radius_ * scale;
    half_thickness_ = base_half_thickness_ * scale;

    TaskManager::RunInRenderThread([this]() {
        if (!GetRenderProxy())
            return;
        GetRenderProxy()->As<LensRenderProxy>()->SetLensParams(sphere_radius_, half_thickness_);
    });
}
} // namespace sparkle

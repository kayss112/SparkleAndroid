#include "scene/component/primitive/BoxPrimitive.h"

#include "io/Mesh.h"
#include "renderer/proxy/BoxRenderProxy.h"

namespace sparkle
{
BoxPrimitive::BoxPrimitive() : MeshPrimitive(Mesh::GetUnitCube())
{
}

std::unique_ptr<RenderProxy> BoxPrimitive::CreateRenderProxy()
{
    return std::make_unique<BoxRenderProxy>(raw_mesh_, node_->GetName(), local_bound_);
}
} // namespace sparkle

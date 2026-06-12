#pragma once

#include "scene/component/RenderableComponent.h"

#include "core/math/AABB.h"

namespace sparkle
{
class Material;

class PrimitiveComponent : public RenderableComponent
{
public:
    // Per-primitive render path. Used by HybridRenderer to route a primitive
    // to either rasterization or ray tracing. For non-hybrid pipelines this is ignored.
    enum class RenderPath : uint8_t
    {
        Default,    // follow the global pipeline
        Rasterize,  // force rasterization
        RayTrace,   // force ray tracing
    };

    PrimitiveComponent(const Vector3 &center, const Vector3 &size);

    ~PrimitiveComponent() override;

    void Tick() override;

    void OnAttach() override;

    void RecreateRenderProxy() override;

    [[nodiscard]] AABB GetWorldBoundingBox() const override
    {
        if (node_->IsTransformDirty())
        {
            node_->UpdateDirtyTransform();
        }
        return world_bound_;
    }

    void SetMaterial(const std::shared_ptr<Material> &material);

    [[nodiscard]] Material *GetMaterial() const
    {
        return material_.get();
    }

    [[nodiscard]] AABB GetLocalBoundingBox() const
    {
        return local_bound_;
    }

    void SetRenderPath(RenderPath path);

    [[nodiscard]] RenderPath GetRenderPath() const
    {
        return render_path_;
    }

protected:
    void OnTransformChange() override;

    std::shared_ptr<Material> material_;

    AABB local_bound_;
    AABB world_bound_;

    RenderPath render_path_ = RenderPath::Default;
};
} // namespace sparkle

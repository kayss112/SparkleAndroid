#pragma once

#include "scene/component/primitive/MeshPrimitive.h"

namespace sparkle
{
class BoxPrimitive : public MeshPrimitive
{
public:
    explicit BoxPrimitive();

protected:
    std::unique_ptr<RenderProxy> CreateRenderProxy() override;
};
} // namespace sparkle

#pragma once

#include "renderer/pass/ScreenQuadPass.h"
#include "renderer/renderer/ForwardRenderer.h"

#include "rhi/RHIBuffer.h"
#include "rhi/RHIComputePass.h"
#include "rhi/RHIPIpelineState.h"
#include "rhi/RHIShader.h"

namespace sparkle
{
class SkyRenderProxy;

// Composite pass that alpha-blends a ray-traced overlay onto an existing color target.
class HybridCompositePass : public ScreenQuadPass
{
public:
    using ScreenQuadPass::ScreenQuadPass;

protected:
    void SetupRenderPass() override;
    void SetupPipeline() override;
    void SetupPixelShader() override;
    void BindPixelShaderResources() override;
};

// HybridRenderer routes primitives to different rendering paths according to their RenderPath:
// - Rasterize/Default primitives are rasterized via the existing forward pipeline.
// - RayTrace primitives are ray-traced into a separate image and composited back onto scene_color.
class HybridRenderer : public ForwardRenderer
{
public:
    HybridRenderer(const RenderConfig &render_config, RHIContext *rhi_context, SceneRenderProxy *scene_render_proxy);

    [[nodiscard]] RenderConfig::Pipeline GetRenderMode() const override
    {
        return RenderConfig::Pipeline::hybrid;
    }

    void InitRenderResources() override;

    ~HybridRenderer() override;

protected:
    void UpdateRayTracingResources() override;
    void RenderRayTracedPrimitives() override;

private:
    void InitRayTracingResources();
    void BindBindlessResources();
    void RebindSkyMap();

    // RT output image: written by compute shader, consumed by composite pass.
    RHIResourceRef<RHIImage> rt_output_image_;

    // Render target wrapping scene_color_ for the composite pass.
    RHIResourceRef<RHIRenderTarget> scene_color_rt_for_composite_;

    RHIResourceRef<RHIBuffer> uniform_buffer_;
    RHIResourceRef<RHIShader> compute_shader_;
    RHIResourceRef<RHIPipelineState> pipeline_state_;
    RHIResourceRef<RHIComputePass> compute_pass_;

    std::unique_ptr<HybridCompositePass> composite_pass_;

    // tracked state for incremental rebinding
    SkyRenderProxy *bound_sky_proxy_ = nullptr;
    uint64_t bound_tlas_generation_ = 0;
    uint32_t dispatched_sample_count_ = 0;
};
} // namespace sparkle

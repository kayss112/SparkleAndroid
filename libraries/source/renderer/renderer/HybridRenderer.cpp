#include "renderer/renderer/HybridRenderer.h"

#include "core/Logger.h"
#include "core/Profiler.h"
#include "renderer/BindlessManager.h"
#include "renderer/proxy/CameraRenderProxy.h"
#include "renderer/proxy/DirectionalLightRenderProxy.h"
#include "renderer/proxy/MeshRenderProxy.h"
#include "renderer/proxy/PrimitiveRenderProxy.h"
#include "renderer/proxy/SceneRenderProxy.h"
#include "renderer/proxy/SkyRenderProxy.h"
#include "renderer/shader/RayTracingComputeShader.h"
#include "rhi/RHI.h"
#include "rhi/RHIRayTracing.h"

namespace sparkle
{
class HybridCompositePixelShader : public RHIShaderInfo
{
    REGISTGER_SHADER(HybridCompositePixelShader, RHIShaderStage::Pixel, "shaders/screen/hybrid_composite.ps.slang",
                     "main")

    BEGIN_SHADER_RESOURCE_TABLE(RHIShaderResourceTable)

    USE_SHADER_RESOURCE(rtOutput, RHIShaderResourceReflection::ResourceType::Texture2D)
    USE_SHADER_RESOURCE(rtOutputSampler, RHIShaderResourceReflection::ResourceType::Sampler)

    END_SHADER_RESOURCE_TABLE
};

void HybridCompositePass::SetupRenderPass()
{
    // Preserve existing scene_color content (rasterized BackGround) and blend RT output on top.
    RHIRenderPass::Attribute pass_attribute;
    pass_attribute.color_load_op = RHIRenderPass::LoadOp::Load;
    pass_attribute.color_store_op = RHIRenderPass::StoreOp::Store;
    pass_attribute.color_initial_layout = RHIImageLayout::ColorOutput;
    pass_attribute.color_final_layout = RHIImageLayout::ColorOutput;
    pass_ = rhi_->CreateRenderPass(pass_attribute, target_, "HybridCompositePass");
}

void HybridCompositePass::SetupPipeline()
{
    pipeline_state_ = rhi_->CreatePipelineState(RHIPipelineState::PipelineType::Graphics, "HybridCompositePipeline");
    pipeline_state_->SetRenderPass(pass_);

    RHIPipelineState::DepthState depth_state;
    depth_state.write_depth = false;
    depth_state.test_state = RHIPipelineState::DepthTestState::Always;
    pipeline_state_->SetDepthState(depth_state);

    RHIPipelineState::BlendState blend_state;
    blend_state.enabled = true;
    blend_state.color_factor_src = RHIPipelineState::BlendFactor::SrcAlpha;
    blend_state.color_factor_dst = RHIPipelineState::BlendFactor::OneMinusSrcAlpha;
    blend_state.color_op = RHIPipelineState::BlendOp::Add;
    blend_state.alpha_factor_src = RHIPipelineState::BlendFactor::One;
    blend_state.alpha_factor_dst = RHIPipelineState::BlendFactor::Zero;
    blend_state.alpha_op = RHIPipelineState::BlendOp::Add;
    pipeline_state_->SetBlendState(blend_state);
}

void HybridCompositePass::SetupPixelShader()
{
    pixel_shader_ = rhi_->CreateShader<HybridCompositePixelShader>();
    pipeline_state_->SetShader<RHIShaderStage::Pixel>(pixel_shader_);
}

void HybridCompositePass::BindPixelShaderResources()
{
    auto *ps_resources = pipeline_state_->GetShaderResource<HybridCompositePixelShader>();
    ps_resources->rtOutput().BindResource(source_texture_->GetDefaultView(rhi_));
    ps_resources->rtOutputSampler().BindResource(source_texture_->GetSampler());
}

HybridRenderer::HybridRenderer(const RenderConfig &render_config, RHIContext *rhi_context,
                               SceneRenderProxy *scene_render_proxy)
    : ForwardRenderer(render_config, rhi_context, scene_render_proxy)
{
    ASSERT(render_config.pipeline == RenderConfig::Pipeline::hybrid);
    ASSERT(rhi_->SupportsHardwareRayTracing());

    Log(Info, "HybridRenderer created.");
}

HybridRenderer::~HybridRenderer() = default;

void HybridRenderer::InitRenderResources()
{
    // 1) Let the forward base set up rasterization passes (also creates tlas_ for hybrid mode).
    ForwardRenderer::InitRenderResources();

    ASSERT_F(tlas_ != nullptr, "ForwardRenderer must allocate TLAS for hybrid mode");

    // 2) Allocate the ray-tracing output image (storage + texture so it can be sampled by composite).
    rt_output_image_ = rhi_->CreateImage(
        RHIImage::Attribute{
            .format = PixelFormat::RGBAFloat,
            .sampler = {.address_mode = RHISampler::SamplerAddressMode::Repeat,
                        .filtering_method_min = RHISampler::FilteringMethod::Nearest,
                        .filtering_method_mag = RHISampler::FilteringMethod::Nearest,
                        .filtering_method_mipmap = RHISampler::FilteringMethod::Nearest},
            .width = image_size_.x(),
            .height = image_size_.y(),
            .usages = RHIImage::ImageUsage::Texture | RHIImage::ImageUsage::UAV,
            .memory_properties = RHIMemoryProperty::DeviceLocal,
            .mip_levels = 1,
            .msaa_samples = 1,
        },
        "HybridRTOutput");

    // 3) RT compute pipeline + bind resources.
    InitRayTracingResources();

    // 4) Composite pass: alpha-blends rt_output_image_ onto scene_color_.
    scene_color_rt_for_composite_ =
        rhi_->CreateRenderTarget({}, scene_color_, nullptr, "HybridSceneColorRTForComposite");

    composite_pass_ =
        PipelinePass::Create<HybridCompositePass>(render_config_, rhi_, rt_output_image_, scene_color_rt_for_composite_);
}

void HybridRenderer::InitRayTracingResources()
{
    uniform_buffer_ = rhi_->CreateBuffer({.size = sizeof(RayTracingComputeShader::UniformBufferData),
                                          .usages = RHIBuffer::BufferUsage::UniformBuffer,
                                          .mem_properties = RHIMemoryProperty::None,
                                          .is_dynamic = true},
                                         "HybridRendererUniformBuffer");

    compute_shader_ = rhi_->CreateShader<RayTracingComputeShader>();

    pipeline_state_ = rhi_->CreatePipelineState(RHIPipelineState::PipelineType::Compute, "HybridRendererPipeline");
    pipeline_state_->SetShader<RHIShaderStage::Compute>(compute_shader_);
    pipeline_state_->Compile();

    auto *cs_resources = pipeline_state_->GetShaderResource<RayTracingComputeShader>();
    cs_resources->ubo().BindResource(uniform_buffer_);
    cs_resources->imageData().BindResource(rt_output_image_->GetDefaultView(rhi_));
    cs_resources->tlas().BindResource(tlas_);

    auto dummy_texture_2d = rhi_->GetOrCreateDummyTexture(RHIImage::Attribute{
        .format = PixelFormat::R8G8B8A8_SRGB,
        .sampler = {.address_mode = RHISampler::SamplerAddressMode::Repeat,
                    .filtering_method_min = RHISampler::FilteringMethod::Nearest,
                    .filtering_method_mag = RHISampler::FilteringMethod::Nearest,
                    .filtering_method_mipmap = RHISampler::FilteringMethod::Nearest},
        .usages = RHIImage::ImageUsage::Texture,
    });

    auto dummy_texture_cube = rhi_->GetOrCreateDummyTexture(RHIImage::Attribute{
        .format = PixelFormat::RGBAFloat16,
        .sampler = {.address_mode = RHISampler::SamplerAddressMode::Repeat,
                    .filtering_method_min = RHISampler::FilteringMethod::Nearest,
                    .filtering_method_mag = RHISampler::FilteringMethod::Nearest,
                    .filtering_method_mipmap = RHISampler::FilteringMethod::Nearest},
        .usages = RHIImage::ImageUsage::Texture,
        .type = RHIImage::ImageType::Image2DCube,
    });

    cs_resources->skyMap().BindResource(dummy_texture_cube->GetDefaultView(rhi_));
    cs_resources->skyMapSampler().BindResource(dummy_texture_cube->GetSampler());

    cs_resources->materialTextureSampler().BindResource(dummy_texture_2d->GetSampler());

    BindBindlessResources();

    compute_pass_ = rhi_->CreateComputePass("HybridRendererComputePass", true);
}

void HybridRenderer::BindBindlessResources()
{
    auto *cs_resources = pipeline_state_->GetShaderResource<RayTracingComputeShader>();
    const auto *bindless_manager = scene_render_proxy_->GetBindlessManager();

    cs_resources->materialIdBuffer().BindResource(bindless_manager->GetMaterialIdBuffer());
    cs_resources->materialBuffer().BindResource(bindless_manager->GetMaterialParameterBuffer());

    cs_resources->textures().BindResource(bindless_manager->GetBindlessBuffer(BindlessResourceType::Texture));
    cs_resources->indexBuffers().BindResource(bindless_manager->GetBindlessBuffer(BindlessResourceType::IndexBuffer));
    cs_resources->vertexBuffers().BindResource(bindless_manager->GetBindlessBuffer(BindlessResourceType::VertexBuffer));
    cs_resources->vertexAttributeBuffers().BindResource(
        bindless_manager->GetBindlessBuffer(BindlessResourceType::VertexAttributeBuffer));
}

void HybridRenderer::RebindSkyMap()
{
    auto *cs_resources = pipeline_state_->GetShaderResource<RayTracingComputeShader>();
    auto *sky_light = scene_render_proxy_->GetSkyLight();

    if ((sky_light != nullptr) && sky_light->GetSkyMap())
    {
        auto sky_map = sky_light->GetSkyMap();
        cs_resources->skyMap().BindResource(sky_map->GetDefaultView(rhi_));
        cs_resources->skyMapSampler().BindResource(sky_map->GetSampler());
    }
    else
    {
        auto dummy_texture = rhi_->GetOrCreateDummyTexture(RHIImage::Attribute{
            .format = PixelFormat::RGBAFloat16,
            .sampler = {.address_mode = RHISampler::SamplerAddressMode::Repeat,
                        .filtering_method_min = RHISampler::FilteringMethod::Nearest,
                        .filtering_method_mag = RHISampler::FilteringMethod::Nearest,
                        .filtering_method_mipmap = RHISampler::FilteringMethod::Nearest},
            .usages = RHIImage::ImageUsage::Texture,
            .type = RHIImage::ImageType::Image2DCube,
        });
        cs_resources->skyMap().BindResource(dummy_texture->GetDefaultView(rhi_));
        cs_resources->skyMapSampler().BindResource(dummy_texture->GetSampler());
    }
}

void HybridRenderer::UpdateRayTracingResources()
{
    PROFILE_SCOPE("HybridRenderer::UpdateRayTracingResources");

    // Bindless buffer may have grown when new materials/meshes are registered.
    if (scene_render_proxy_->GetBindlessManager()->IsBufferDirty())
    {
        BindBindlessResources();
    }

    // Sky light proxy can change at runtime (e.g. when a new sky map is loaded).
    auto *sky_light = scene_render_proxy_->GetSkyLight();
    if (sky_light != bound_sky_proxy_)
    {
        bound_sky_proxy_ = sky_light;
        RebindSkyMap();
        scene_render_proxy_->GetCamera()->MarkPixelDirty();
        dispatched_sample_count_ = 0;
    }

    // ForwardRenderer::HandleSceneChanges already (re)builds tlas_ when needed. Rebind it to be safe
    // -- BindResource is cheap and idempotent for unchanged resources.
    auto *cs_resources = pipeline_state_->GetShaderResource<RayTracingComputeShader>();
    cs_resources->tlas().BindResource(tlas_, true);

    // Upload uniform buffer (camera + sky + dir light + spp).
    auto *camera = scene_render_proxy_->GetCamera();

    if (camera->NeedClear())
    {
        camera->ClearPixels();
        dispatched_sample_count_ = 0;
    }

    const uint32_t spp = 1u; // hybrid mode renders one sample per pixel per frame for now

    RayTracingComputeShader::UniformBufferData ubo{
        .camera = camera->GetUniformBufferData(render_config_),
        .time_seed = dispatched_sample_count_,
        .total_sample_count = camera->GetCumulatedSampleCount(),
        .spp = spp,
        .enable_nee = render_config_.enable_nee ? 1u : 0u,
    };

    if (sky_light)
    {
        ubo.sky_light = sky_light->GetRenderData();
    }
    auto *dir_light = scene_render_proxy_->GetDirectionalLight();
    if (dir_light)
    {
        ubo.dir_light = dir_light->GetRenderData();
    }

    uniform_buffer_->Upload(rhi_, &ubo);

    dispatched_sample_count_ += spp;
    camera->AccumulateSample(spp);

    if (composite_pass_)
    {
        composite_pass_->UpdateFrameData(render_config_, scene_render_proxy_);
    }
}

void HybridRenderer::RenderRayTracedPrimitives()
{
    PROFILE_SCOPE("HybridRenderer::RenderRayTracedPrimitives");

    // Skip the RT pass when there is nothing to ray trace this frame. This avoids dispatching
    // with an empty TLAS which would crash the driver.
    if (scene_render_proxy_->GetRayTracePrimitives().empty())
    {
        return;
    }

    rt_output_image_->Transition({.target_layout = RHIImageLayout::StorageWrite,
                                  .after_stage = RHIPipelineStage::Top,
                                  .before_stage = RHIPipelineStage::ComputeShader});

    rhi_->BeginComputePass(compute_pass_);
    rhi_->DispatchCompute(pipeline_state_, {image_size_.x(), image_size_.y(), 1u}, {16u, 16u, 1u});
    rhi_->EndComputePass(compute_pass_);

    rt_output_image_->Transition({.target_layout = RHIImageLayout::Read,
                                  .after_stage = RHIPipelineStage::ComputeShader,
                                  .before_stage = RHIPipelineStage::PixelShader});

    // Composite RT output onto scene_color via alpha blend.
    composite_pass_->Render();
}
} // namespace sparkle

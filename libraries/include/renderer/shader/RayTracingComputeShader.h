#pragma once

#include "renderer/proxy/CameraRenderProxy.h"
#include "renderer/proxy/DirectionalLightRenderProxy.h"
#include "renderer/proxy/SkyRenderProxy.h"
#include "rhi/RHIShader.h"

namespace sparkle
{
// Ray tracing compute shader shared by GPURenderer and HybridRenderer.
// Inputs an acceleration structure (TLAS) plus material/scene resources and writes the
// path-traced result into a storage image (`imageData`).
class RayTracingComputeShader : public RHIShaderInfo
{
    REGISTGER_SHADER(RayTracingComputeShader, RHIShaderStage::Compute, "shaders/ray_trace/ray_trace.cs.slang", "main")

    BEGIN_SHADER_RESOURCE_TABLE(RHIShaderResourceTable)

    USE_SHADER_RESOURCE(tlas, RHIShaderResourceReflection::ResourceType::AccelerationStructure)
    USE_SHADER_RESOURCE(ubo, RHIShaderResourceReflection::ResourceType::DynamicUniformBuffer)
    USE_SHADER_RESOURCE(imageData, RHIShaderResourceReflection::ResourceType::StorageImage2D)
    USE_SHADER_RESOURCE(materialIdBuffer, RHIShaderResourceReflection::ResourceType::StorageBuffer)
    USE_SHADER_RESOURCE(materialBuffer, RHIShaderResourceReflection::ResourceType::StorageBuffer)
    USE_SHADER_RESOURCE(skyMap, RHIShaderResourceReflection::ResourceType::Texture2D)
    USE_SHADER_RESOURCE(skyMapSampler, RHIShaderResourceReflection::ResourceType::Sampler)
    USE_SHADER_RESOURCE(materialTextureSampler, RHIShaderResourceReflection::ResourceType::Sampler)

    USE_SHADER_RESOURCE_BINDLESS(textures, RHIShaderResourceReflection::ResourceType::Texture2D)
    USE_SHADER_RESOURCE_BINDLESS(indexBuffers, RHIShaderResourceReflection::ResourceType::StorageBuffer)
    USE_SHADER_RESOURCE_BINDLESS(vertexBuffers, RHIShaderResourceReflection::ResourceType::StorageBuffer)
    USE_SHADER_RESOURCE_BINDLESS(vertexAttributeBuffers, RHIShaderResourceReflection::ResourceType::StorageBuffer)

    END_SHADER_RESOURCE_TABLE

    struct UniformBufferData
    {
        CameraRenderProxy::UniformBufferData camera;
        SkyRenderProxy::UniformBufferData sky_light = {};
        DirectionalLightRenderProxy::UniformBufferData dir_light = {};
        uint32_t time_seed;
        float output_limit = CameraRenderProxy::OutputLimit;
        uint32_t total_sample_count;
        uint32_t spp;
        uint32_t enable_nee;
    };
};
} // namespace sparkle

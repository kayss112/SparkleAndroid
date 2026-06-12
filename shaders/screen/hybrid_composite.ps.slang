#include "screen.h.slang"

// Composite ray-traced output onto scene_color via alpha blend.
// rtOutput.alpha indicates whether ray tracing hit a RT primitive (1.0) or missed (0.0).
[[vk::binding(0, 1)]] Texture2D rtOutput;
[[vk::binding(1, 1)]] SamplerState rtOutputSampler;

[shader("fragment")]
float4 main(VSInterpolant input) : SV_TARGET
{
    return rtOutput.Sample(rtOutputSampler, input.uv);
}

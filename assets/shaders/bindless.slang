module bindless;

[[vk::binding(0, 0)]]
Texture2D<float4> sampled_2d[];

[[vk::binding(1, 0)]]
SamplerState samplers[];

[[vk::binding(2, 0)]]
SamplerComparisonState comparison_samplers[];
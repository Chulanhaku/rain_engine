cbuffer ObjectConstants : register(b0)
{
    row_major float4x4 world_matrix;
    row_major float4x4 world_view_projection;
    row_major float4x4 normal_matrix;
};

cbuffer SceneConstants : register(b1)
{
    float4 light_direction;
    float4 light_color;
    float4 ambient_color;
    float4 camera_position;
};

cbuffer MaterialConstants : register(b2)
{
    float4 base_color;          // Linear RGB, linear alpha.
    float4 material_parameters; // Metallic, roughness, alpha cutoff, manual sRGB decode.
    float4 material_options;    // Albedo map, MR map, blended, double sided.
};

Texture2D albedo_texture : register(t0);
Texture2D metallic_roughness_texture : register(t1);
SamplerState material_sampler : register(s0);

static const float PI = 3.14159265359f;

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    centroid float3 world_position : POSITION1;
    centroid float3 normal : NORMAL;
    centroid float2 uv : TEXCOORD;
};

float3 safe_normalize(float3 value, float3 fallback)
{
    float magnitude_squared = dot(value, value);
    return magnitude_squared > 1.0e-12f ? value * rsqrt(max(magnitude_squared, 1.0e-12f)) : fallback;
}

VertexOutput vertex_main(VertexInput input)
{
    VertexOutput output;
    float4 local_position = float4(input.position, 1.0f);
    output.position = mul(local_position, world_view_projection);
    output.world_position = mul(local_position, world_matrix).xyz;
    output.normal = mul(float4(input.normal, 0.0f), normal_matrix).xyz;
    output.uv = input.uv;
    return output;
}

float3 srgb_to_linear(float3 color)
{
    return float3(color.r <= 0.04045f ? color.r / 12.92f : pow(abs((color.r + 0.055f) / 1.055f), 2.4f),
                  color.g <= 0.04045f ? color.g / 12.92f : pow(abs((color.g + 0.055f) / 1.055f), 2.4f),
                  color.b <= 0.04045f ? color.b / 12.92f : pow(abs((color.b + 0.055f) / 1.055f), 2.4f));
}

float distribution_ggx(float normal_dot_halfway, float roughness)
{
    float alpha = roughness * roughness;
    float alpha_squared = alpha * alpha;
    float nh_squared = normal_dot_halfway * normal_dot_halfway;
    // This form avoids cancellation when roughness and the highlight angle are small.
    float denominator = (1.0f - nh_squared) + nh_squared * alpha_squared;
    return alpha_squared / max(PI * denominator * denominator, 1.0e-12f);
}

float geometry_schlick_ggx(float normal_dot_direction, float roughness)
{
    float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
    return normal_dot_direction / max(normal_dot_direction * (1.0f - k) + k, 1.0e-6f);
}

float3 fresnel_schlick(float cosine_theta, float3 f0)
{
    return f0 + (1.0f - f0) * pow(1.0f - saturate(cosine_theta), 5.0f);
}

float4 pixel_main(VertexOutput input, bool front_face : SV_IsFrontFace) : SV_Target
{
    float4 albedo = base_color;
    if (material_options.x > 0.5f) {
        float4 sampled = albedo_texture.Sample(material_sampler, input.uv);
        if (material_parameters.w > 0.5f) sampled.rgb = srgb_to_linear(sampled.rgb);
        albedo *= sampled;
    }
    albedo = saturate(albedo);
    if (material_parameters.z >= 0.0f) clip(albedo.a - material_parameters.z);

    float metallic = material_parameters.x;
    float roughness = material_parameters.y;
    if (material_options.y > 0.5f) {
        float4 mr = metallic_roughness_texture.Sample(material_sampler, input.uv);
        roughness *= mr.g;
        metallic *= mr.b;
    }
    metallic = saturate(metallic);
    roughness = clamp(roughness, 0.04f, 1.0f);

    float3 normal = safe_normalize(input.normal, float3(0, 1, 0));
    if (material_options.w > 0.5f && !front_face) normal = -normal;
    float3 view_direction = safe_normalize(camera_position.xyz - input.world_position, normal);
    // A disabled/zero direction contributes no directional lighting.
    float3 light = safe_normalize(-light_direction.xyz, float3(0, 0, 0));
    float3 halfway = safe_normalize(view_direction + light, normal);
    float nv = saturate(dot(normal, view_direction));
    float nl = saturate(dot(normal, light));
    float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo.rgb, metallic);
    float3 fresnel = fresnel_schlick(dot(halfway, view_direction), f0);
    float distribution = distribution_ggx(saturate(dot(normal, halfway)), roughness);
    float geometry = geometry_schlick_ggx(nv, roughness) * geometry_schlick_ggx(nl, roughness);
    float3 specular = distribution * geometry * fresnel / max(4.0f * nv * nl, 1.0e-6f);
    float3 diffuse_weight = (1.0f - fresnel) * (1.0f - metallic);
    float3 direct = (diffuse_weight * albedo.rgb / PI + specular) * max(light_color.rgb, 0.0f) * nl;
    // Diffuse ambient approximation; environment specular needs a future IBL pass.
    float3 ambient = max(ambient_color.rgb, 0.0f) * albedo.rgb * (1.0f - metallic);
    float3 radiance = max(direct + ambient, 0.0f);
    float3 mapped = radiance / (radiance + 1.0f);
    // The 3D pipeline writes through an sRGB RTV; blending happens in linear space.
    return float4(mapped, material_options.z > 0.5f ? albedo.a : 1.0f);
}

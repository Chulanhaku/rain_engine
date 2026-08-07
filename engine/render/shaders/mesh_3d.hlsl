cbuffer ObjectConstants : register(b0)
{
    row_major float4x4 world_matrix;
    row_major float4x4 world_view_projection;
};

cbuffer SceneConstant : register(b1)
{
    float4 light_direction;
    float4 light_color;
    float4 ambient_color;
    float4 camera_position;
}

cbuffer MaterialConstant : register(b2)
{
    float4 base_color;
    float4 material_options;
}

Texture2D albedo_texture : register(t0);

SamplerState albedo_sampler : register(s0);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 world_position : POSITION1;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    
};

VertexOutput vertex_main(VertexInput input)
{
    VertexOutput output;
    float4 local_position = float4(input.position, 1.0f);
    
    float4 world_position = mul(local_position, world_matrix);
    
    output.position = mul(local_position, world_view_projection);
    
    output.world_position = world_position.xyz;
    
    output.normal = normalize(mul(float4(input.normal, 0.0f), world_matrix).xyz);
    
    output.uv = input.uv;
    
    return output;

}

float4 pixel_main(VertexOutput input) : SV_Target{
    const float has_texture = material_options.x;
    
    float4 texture_color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    
    if (has_texture > 0.5f)
    {
        texture_color = albedo_texture.Sample(albedo_sampler,input.uv);
    }
    
    const float3 normal = normalize(input.normal);
    
    const float3 incoming_light = normalize(-light_direction.xyz);
    
    const float diffuse_factor = saturate(dot(normal, incoming_light));
    
    const float3 lighting = ambient_color.rgb + light_color.rgb * diffuse_factor;
    
    const float4 result = texture_color * base_color;
    
    return float4(result.rgb * lighting, result.a);
    
}


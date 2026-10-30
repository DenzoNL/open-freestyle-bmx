// Direct3D 7 transformed and lit vertices (D3DFVF_TLVERTEX) on SDL3 GPU: the vertex shader only maps screen
// pixels to clip space, the fragment shader does stage 0 of D3D7's fixed function texturing (colour and alpha
// op select / modulate), vertex fog and the alpha test. Compiled offline by tools/shaders.sh (docs/SDL3-DRIVER.md).
// Bindings follow SDL3 GPU's HLSL conventions: vertex uniforms b0 space1, fragment texture/sampler space2,
// fragment uniforms b0 space3.

struct VSInput
{
    float4 pos : TEXCOORD0;      // sx, sy, sz, rhw
    float4 color : TEXCOORD1;    // D3DCOLOR as bytes B, G, R, A
    float4 specular : TEXCOORD2; // alpha: fog factor
    float2 uv : TEXCOORD3;
};

struct VSOutput
{
    float4 color : TEXCOORD0;
    float4 specular : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float4 pos : SV_Position;
};

cbuffer VSUniforms : register(b0, space1)
{
    float4 screen; // 2 / width, -2 / height of the render target, unused, unused
};

VSOutput vsMain(VSInput input)
{
    VSOutput output;
    // D3D7 puts pixel centres on integer coordinates, D3D10 and later at +0.5.
    float2 ndc = float2((input.pos.x + 0.5) * screen.x - 1.0, (input.pos.y + 0.5) * screen.y + 1.0);
    float w = input.pos.w > 0.0 ? 1.0 / input.pos.w : 1.0;
    output.pos = float4(ndc * w, input.pos.z * w, w);
    output.color = input.color.zyxw;
    output.specular = input.specular.zyxw;
    output.uv = input.uv;
    return output;
}

[[vk::combinedImageSampler]][[vk::binding(0, 2)]] Texture2D<float4> tex : register(t0, space2);
[[vk::combinedImageSampler]][[vk::binding(0, 2)]] SamplerState smp : register(s0, space2);

cbuffer PSUniforms : register(b0, space3)
{
    int4 ops;        // colour op, alpha op (D3DTOP: 2 arg1 = texture, 3 arg2 = diffuse, 4 modulate), texture, alpha func
    float4 params;   // alpha ref (0..1), fog on, specular on, unused
    float4 fogColor;
};

float4 psMain(VSOutput input) : SV_Target0
{
    float4 diffuse = input.color;
    float4 texel = ops.z != 0 ? tex.Sample(smp, input.uv) : float4(1.0, 1.0, 1.0, 1.0);
    float4 result;
    result.rgb = ops.x == 2 ? texel.rgb : ops.x == 3 ? diffuse.rgb : texel.rgb * diffuse.rgb;
    result.a = ops.y == 2 ? texel.a : ops.y == 3 ? diffuse.a : texel.a * diffuse.a;
    if (params.z != 0.0)
    {
        result.rgb = saturate(result.rgb + input.specular.rgb);
    }
    if (params.y != 0.0)
    {
        result.rgb = lerp(fogColor.rgb, result.rgb, input.specular.a);
    }
    // D3DCMP: 1 never, 2 less, 3 equal, 4 less equal, 5 greater, 6 not equal, 7 greater equal, 8 always.
    // The reference is a byte; compare at the 8-bit precision of the target, as the hardware did.
    int a = (int)round(saturate(result.a) * 255.0);
    int ref = (int)round(params.x * 255.0);
    bool pass = ops.w == 1 ? false : ops.w == 2 ? a < ref : ops.w == 3 ? a == ref : ops.w == 4 ? a <= ref
              : ops.w == 5 ? a > ref : ops.w == 6 ? a != ref : ops.w == 7 ? a >= ref : true;
    if (!pass)
    {
        discard;
    }
    return result;
}

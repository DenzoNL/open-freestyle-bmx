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
    float4 params;   // alpha ref (0..1), fog on, specular on, sampling mode (0 as bound, 1 linear, 2 sharp, 3 blur)
    float4 fogColor; // alpha: brightness of the blur mode
#ifdef UI
    float4 uvBox;    // the draw's texture coordinates: min u, min v, max u, max v (draw.c)
#endif
};

// The UI variant (compiled with -DUI, used for the 2D funnel's draws when ui_filter or menus = 2 asks for it; the
// plain psMain stays as it was, so default frames are byte identical).
#ifdef UI
// Sharp-bilinear (docs/WIDESCREEN.md W7): nearest inside a texel, linear over one output pixel at its edges.
// The sampler is linear; s = output pixels per texel (at least 1) comes from the screen space derivatives.
float2 sharpUv(float2 uv)
{
    uint tw, th;
    tex.GetDimensions(tw, th);
    float2 size = float2(tw, th);
    float2 t = uv * size;
    float2 dx = abs(ddx(t)), dy = abs(ddy(t));
    float2 s = 1.0 / max(max(dx, dy), 1.0e-4);
    s = max(s, 1.0);
    float2 centre = floor(t) + 0.5;
    return (centre + clamp((t - centre) * s, -0.5, 0.5)) / size;
}

// Linear and sharp sampling read up to half a texel beyond the texel the pixel lies in; at the edge of a glyph or
// sprite that is its neighbour in the atlas (stray marks under digits, cut outlines). The game's nearest sampling
// never leaves the draw's texture box, so filtered samples are kept half a texel inside it.
float2 insideBox(float2 uv)
{
    uint tw, th;
    tex.GetDimensions(tw, th);
    float2 size = float2(tw, th);
    float2 lo = uvBox.xy * size + 0.5, hi = uvBox.zw * size - 0.5;
    float2 mid = (lo + hi) * 0.5;
    return clamp(uv * size, min(lo, mid), max(hi, mid)) / size;
}

// The fill behind a pillarboxed backdrop (menus = 2): 5 x 5 taps four texels apart on the linear sampler.
float4 blurTexel(float2 uv)
{
    uint tw, th;
    tex.GetDimensions(tw, th);
    float2 step = 4.0 / float2(tw, th);
    float4 sum = float4(0.0, 0.0, 0.0, 0.0);
    float total = 0.0;
    [unroll] for (int j = -2; j <= 2; j++)
    {
        [unroll] for (int i = -2; i <= 2; i++)
        {
            float w = (3.0 - abs(i)) * (3.0 - abs(j));
            sum += w * tex.SampleLevel(smp, uv + float2(i, j) * step, 0);
            total += w;
        }
    }
    return sum / total;
}
#endif

float4 psMain(VSOutput input) : SV_Target0
{
    float4 diffuse = input.color;
#ifdef UI
    float2 uv = params.w == 2.0 ? sharpUv(input.uv) : input.uv;
    if (params.w == 1.0 || params.w == 2.0)
    {
        uv = insideBox(uv);
    }
    float4 texel = float4(1.0, 1.0, 1.0, 1.0);
    if (ops.z != 0)
    {
        texel = params.w == 3.0 ? blurTexel(uv) : tex.Sample(smp, uv);
    }
#else
    float4 texel = ops.z != 0 ? tex.Sample(smp, input.uv) : float4(1.0, 1.0, 1.0, 1.0);
#endif
    float4 result;
    result.rgb = ops.x == 2 ? texel.rgb : ops.x == 3 ? diffuse.rgb : texel.rgb * diffuse.rgb;
    result.a = ops.y == 2 ? texel.a : ops.y == 3 ? diffuse.a : texel.a * diffuse.a;
#ifdef UI
    if (params.w == 3.0)
    {
        result.rgb *= fogColor.a;
    }
#endif
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

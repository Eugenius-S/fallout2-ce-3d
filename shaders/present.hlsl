// Final pass: linear HDR color (or the NGX output) -> sRGB swapchain.
// Fullscreen triangle generated from SV_VertexID, no vertex buffer.

Texture2D srcTex : register(t0);
SamplerState linearSamp : register(s0);

struct VSOut {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOut vsMain(uint id : SV_VertexID) {
    VSOut o;
    float2 xy = float2((id == 1) ? 3.0 : -1.0, (id == 2) ? 3.0 : -1.0);
    o.pos = float4(xy, 0.0, 1.0);
    o.uv = xy * float2(0.5, -0.5) + 0.5;
    return o;
}

float4 psMain(VSOut i) : SV_Target {
    float3 c = srcTex.Sample(linearSamp, i.uv).rgb;
    // One linear->sRGB transform on the way out (spec section 5).
    c = (c <= 0.0031308) ? 12.92 * c : 1.055 * pow(max(c, 1e-5), 1.0 / 2.4) - 0.055;
    return float4(saturate(c), 1.0);
}

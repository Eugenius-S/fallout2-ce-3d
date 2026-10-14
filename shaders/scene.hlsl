// G-buffer pass: linear color (R16G16B16A16F), motion vectors (RG16F, pixels,
// previous - current, X right / Y down) and device depth (D32).
// Motion follows spec section 5 exactly:
//   currUV = (currClip.x/w*0.5+0.5, 0.5 - currClip.y/w*0.5)   (same for prev)
//   motion = (prevUV - currUV) * renderSize

cbuffer FrameCB : register(b0) {
    float4x4 viewProj;
    float4x4 prevViewProj;
    float2 renderSize;   // pixels
    uint objectId;       // 0 floor, 1 boxA, 2 boxB, 3 alpha card
    float pad;
};
Texture2D albedoTex : register(t0);
SamplerState linearSamp : register(s0);

struct VSOut {
    float4 pos : SV_POSITION;
    float4 currClip : TEXCOORD0;
    float4 prevClip : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float3 color : TEXCOORD3;
};

VSOut vsMain(float3 position : POSITION, float2 uvIn : UV, float3 colorIn : COLOR) {
    VSOut o;
    float4 world = float4(position, 1.0);
    o.currClip = mul(world, viewProj);
    o.prevClip = mul(world, prevViewProj);
    o.pos = o.currClip;
    o.uv = uvIn;
    o.color = colorIn;
    return o;
}

struct PSOut {
    float4 color : SV_Target0;
    float2 motion : SV_Target1;
};

PSOut psMain(VSOut i) {
    PSOut o;
    float4 c = float4(i.color, 1.0);
    if (objectId == 3) {
        // Alpha card: hard hole, alpha-tested geometry writes no depth/motion.
        float2 d = i.uv - 0.5;
        if (dot(d, d) < 0.0625) discard; // central circle r=0.25 in UV space
    } else {
        c.rgb *= albedoTex.Sample(linearSamp, i.uv).rgb;
    }
    o.color = float4(c.rgb, 1.0);

    float2 currUV = float2(i.currClip.x / i.currClip.w * 0.5 + 0.5,
                           0.5 - i.currClip.y / i.currClip.w * 0.5);
    float2 prevUV = float2(i.prevClip.x / i.prevClip.w * 0.5 + 0.5,
                           0.5 - i.prevClip.y / i.prevClip.w * 0.5);
    o.motion = (prevUV - currUV) * renderSize;
    return o;
}

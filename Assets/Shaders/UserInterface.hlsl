#include "CoreDefines.hlsli"

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-64
    float4x4 ProjectionMatrix;
SHADER_CONSTANT_BLOCK_END

// ------------------------------------------------------------------------------------------------
// Textured quads (text, images, square fills)
// ------------------------------------------------------------------------------------------------

struct FVSInput
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR0;
};

struct FVSOutput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output;
    Output.Position = mul(Constants.ProjectionMatrix, float4(Input.Position.xy, 0.0f, 1.0f));
    Output.Color    = Input.Color;
    Output.TexCoord = Input.TexCoord;
    return Output;
}

struct FVSTextInput
{
    float2 Position    : POSITION;
    float2 Size        : TEXCOORD0;
    float2 MinTexCoord : TEXCOORD1;
    float2 MaxTexCoord : TEXCOORD2;
    float4 Color       : COLOR0;
};

FVSOutput VSText(FVSTextInput Input, uint VertexId : SV_VertexID)
{
    static const float2 Corners[6] =
    {
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 1.0f),
    };

    const float2 Corner = Corners[VertexId];

    FVSOutput Output;
    Output.Position = mul(Constants.ProjectionMatrix, float4(Input.Position + (Input.Size * Corner), 0.0f, 1.0f));
    Output.Color    = Input.Color;
    Output.TexCoord = lerp(Input.MinTexCoord, Input.MaxTexCoord, Corner);
    return Output;
}

struct FPSInput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

SamplerState Sampler0 : register(s0);
Texture2D    Texture0 : register(t0);

float4 PSMain(FPSInput Input) : SV_Target
{
    float4 OutColor = Input.Color * Texture0.Sample(Sampler0, Input.TexCoord);
    return float4(OutColor.rgb * OutColor.a, OutColor.a);
}

// ------------------------------------------------------------------------------------------------
// SDF rounded fills, strokes, and corner wedges
// ------------------------------------------------------------------------------------------------

struct FVSShapeInput
{
    float2 Position    : POSITION;
    float4 Color       : COLOR0;
    float2 DrawSize    : TEXCOORD0;
    float2 LocalOrigin : TEXCOORD1;
    float2 RectSize    : TEXCOORD2;
    float4 Radii       : TEXCOORD3;
    float2 ShapeData   : TEXCOORD4;
};

struct FVSShapeOutput
{
    float4 Position  : SV_POSITION;
    float4 Color     : COLOR0;
    float2 LocalPos  : TEXCOORD0;
    float2 RectSize  : TEXCOORD1;
    float4 Radii     : TEXCOORD2;
    float2 ShapeData : TEXCOORD3;
};

FVSShapeOutput VSShape(FVSShapeInput Input, uint VertexId : SV_VertexID)
{
    static const float2 Corners[6] =
    {
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 1.0f),
    };

    const float2 Corner = Corners[VertexId];
    FVSShapeOutput Output;
    Output.Position  = mul(Constants.ProjectionMatrix, float4(Input.Position + (Input.DrawSize * Corner), 0.0f, 1.0f));
    Output.Color     = Input.Color;
    Output.LocalPos  = Input.LocalOrigin + (Input.DrawSize * Corner);
    Output.RectSize  = Input.RectSize;
    Output.Radii     = Input.Radii;
    Output.ShapeData = Input.ShapeData;
    return Output;
}

float SignedDistanceToRoundedBox(float2 P, float2 HalfSize, float4 Radii)
{
    // Radii: x=TL y=TR z=BR w=BL. P.y > 0 is toward the bottom of the rect.
    // Narrow the four radii down to the corner this sample belongs to, by side first and then by row.
    float4 SideRadii = float4(Radii.z, Radii.y, Radii.w, Radii.x);
    SideRadii.xy = (P.x > 0.0f) ? SideRadii.xy : SideRadii.zw;

    const float CornerRadius = (P.y > 0.0f) ? SideRadii.x : SideRadii.y;

    float2 Q = abs(P) - HalfSize + CornerRadius;
    return min(max(Q.x, Q.y), 0.0f) + length(max(Q, 0.0f)) - CornerRadius;
}

float4 PSShape(FVSShapeOutput Input) : SV_Target
{
    const float Thickness = Input.ShapeData.x;
    const float ShapeKind = Input.ShapeData.y;
    const float Fringe    = 1.0f;

    float Distance;
    if (ShapeKind > 1.5f)
    {
        Distance = length(Input.LocalPos - Input.Radii.xy) - Input.Radii.z;
    }
    else
    {
        const float2 Centered = Input.LocalPos - (Input.RectSize * 0.5f);
        Distance = SignedDistanceToRoundedBox(Centered, Input.RectSize * 0.5f, Input.Radii);

        if (ShapeKind > 0.5f)
        {
            // Inset the band so the stroke sits inside the bounds like the tessellated ring did, otherwise a
            // 1px border straddles the edge and covers two pixels at half coverage each.
            const float HalfThickness = Thickness * 0.5f;
            Distance = abs(Distance + HalfThickness) - HalfThickness;
        }
    }

    float Coverage = 1.0f - smoothstep(-Fringe * 0.5f, Fringe * 0.5f, Distance);
    float4 OutColor = Input.Color;
    OutColor.a *= Coverage;
    return float4(OutColor.rgb * OutColor.a, OutColor.a);
}

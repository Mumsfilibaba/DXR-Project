#include "Structs.hlsli"
#include "DepthHelpers.hlsli"
#include "ColorSpaceTransforms.hlsli"

Texture2D TonemappedImage : register(t0);
Texture2D<float> SelectionRing : register(t1);
Texture2D<float> NoJitterDepth : register(t2);

SamplerState PointSampler  : register(s0);
SamplerState LinearSampler : register(s1);

ConstantBuffer<FCamera> CameraBuffer : register(b0);

SHADER_CONSTANT_BLOCK_BEGIN
    int    EnableSelectionOutline;
    int    EnableGrid;
    float  OutlineAlpha;
    float  GridPlaneY;

    float  GridMinorSize;
    float  GridMajorSize;
    float  GridMinorWidth;
    float  GridMajorWidth;

    float3 OutlineColor;
    float  GridFadeDistance;

    float3 GridMinorColor;
    float  GridMinorAlpha;

    float3 GridMajorColor;
    float  GridMajorAlpha;

    float  GridHorizonFade;
    float  GridDepthBias;
    float  GridMaxTraceDistance;
    float  Padding2;
SHADER_CONSTANT_BLOCK_END

float GridLinesAA(float2 CoordWS, float SizeWS, float Width)
{
    const float Size = max(SizeWS, 1e-6);
    const float W    = max(Width, 1e-3);

    const float2 UV = CoordWS / Size;
    const float2 FW = max(fwidth(UV), float2(1e-6, 1e-6));

    // Distance to nearest grid line in UV space (0 at line, 0.5 at cell center).
    const float2 Dist = abs(frac(UV - 0.5) - 0.5);

    // Coverage estimate in screen space.
    const float2 A = Dist / (FW * W);
    return 1.0 - saturate(min(A.x, A.y));
}

float4 Main(float2 TexCoord : TEXCOORD0, float4 Position : SV_Position) : SV_TARGET0
{
    float3 Color = TonemappedImage.Sample(PointSampler, TexCoord).rgb;

    if (Constants.EnableGrid != 0)
    {
        const float2 NDC = float2(TexCoord.x * 2.0 - 1.0, (1.0 - TexCoord.y) * 2.0 - 1.0);

        const float3 RayOriginWS = CameraBuffer.PositionWS;

        // Reconstruct a per-pixel view ray using the unjittered projection, then rotate it into world space.
        const float4 ClipFar  = float4(NDC.x, NDC.y, 1.0, 1.0);
        const float4 ViewFar4 = mul(ClipFar, CameraBuffer.ProjectionInvUnjittered);
        const float3 ViewFar  = ViewFar4.xyz / max(ViewFar4.w, 1e-6);

        const float3 RayDirVS = normalize(ViewFar);
        const float3 RayDirWS = normalize(mul(float4(RayDirVS, 0.0), CameraBuffer.ViewInv).xyz);

        const float Denom = RayDirWS.y;
        if (abs(Denom) > 1e-6)
        {
            const float TPlane = (Constants.GridPlaneY - RayOriginWS.y) / Denom;
            if (TPlane > 0.0)
            {
                if (Constants.GridMaxTraceDistance > 0.0 && TPlane > Constants.GridMaxTraceDistance)
                {
                    // Hard cut-off for performance (avoids depth fetch + line math).
                }
                else
                {
                    // Apply distance + horizon fade before doing depth work (saves perf on far pixels).
                    float Fade = 1.0;
                    if (Constants.GridFadeDistance > 0.0)
                    {
                        Fade *= saturate(1.0 - (TPlane / Constants.GridFadeDistance));
                    }

                if (Constants.GridHorizonFade > 0.0)
                {
                    Fade *= saturate(abs(Denom) * Constants.GridHorizonFade);
                }

                    if (Fade > 0.0)
                    {
                        // Depth-occlusion (NoJitterDepth stores the closest rendered surface).
                        // Compare distances along the view ray without reconstructing full world-space positions.
                        const uint2 Pixel      = uint2(Position.xy);
                        const float SceneDepth = NoJitterDepth.Load(int3(Pixel, 0)).r;

                        float Visibility = 1.0;
                        if (SceneDepth < 1.0)
                        {
                            const float ViewZ = Depth_ProjToView(SceneDepth, CameraBuffer.ProjectionInvUnjittered);
                            const float RayZ  = RayDirVS.z;

                            // If the ray is near-parallel to the view Z axis, depth-to-distance becomes unstable.
                            // In that case, rely on horizon fade + line AA.
                            if (abs(RayZ) > 1e-6)
                            {
                                const float TScene = ViewZ / RayZ;
                                const float Delta  = TScene - TPlane;

                                // Fade-out near depth intersections to avoid z-fighting/flicker.
                                // DepthBias controls how far behind geometry the grid must be before it disappears.
                                const float Bias = max(Constants.GridDepthBias, 0.0);
                                const float Soft = max(Bias * 4.0, abs(fwidth(TPlane)) * 2.0);
                                Visibility = saturate((Delta - Bias) / max(Soft, 1e-5));
                            }
                        }

                        if (Visibility > 0.0)
                        {
                            // Contrast-aware grid: pick a lighter/darker grid tint depending on scene luminance.
                            // Uses the already-tonemapped scene color behind the grid (linear).
                            const float3 SceneColor = Color;
                            const float  SceneLuma  = dot(SceneColor, float3(0.2126, 0.7152, 0.0722));

                            // Dark backgrounds -> brighter grid, bright backgrounds -> darker grid.
                            const float ContrastScale = lerp(1.65, 0.55, saturate(SceneLuma));

                            // Optional extra alpha when background is mid-gray (worst contrast).
                            const float MidGray    = saturate(1.0 - abs(SceneLuma - 0.5) * 2.0);
                            const float AlphaScale = 1.0 + (MidGray * 0.35);

                            const float3 MinorColor = saturate(Constants.GridMinorColor * ContrastScale);
                            const float3 MajorColor = saturate(Constants.GridMajorColor * ContrastScale);

                            const float3 HitWS   = RayOriginWS + (RayDirWS * TPlane);
                            const float2 CoordWS = HitWS.xz;

                            const float Major = GridLinesAA(CoordWS, Constants.GridMajorSize, Constants.GridMajorWidth);
                            float Minor = GridLinesAA(CoordWS, Constants.GridMinorSize, Constants.GridMinorWidth);
                            Minor *= (1.0 - Major);

                            const float MinorA = Minor * Constants.GridMinorAlpha * AlphaScale;
                            const float MajorA = Major * Constants.GridMajorAlpha * AlphaScale;
                            const float Alpha  = saturate((MinorA + MajorA) * Fade * Visibility);

                            const float3 GridPremul = ((MinorA * MinorColor) + (MajorA * MajorColor)) * Fade * Visibility;
                            Color = (Color * (1.0 - Alpha)) + GridPremul;
                        }
                    }
                }
            }
        }
    }

    if (Constants.EnableSelectionOutline != 0)
    {
        const float Ring = SelectionRing.Sample(LinearSampler, TexCoord).r;
        Color = lerp(Color, Constants.OutlineColor, Ring * Constants.OutlineAlpha);
    }

    Color = LinearToSRGB(Color);
    return float4(Color, 1.0);
}

#include "Structs.hlsli"
#include "DepthHelpers.hlsli"
#include "ColorSpaceTransforms.hlsli"

Texture2D<float4> TonemappedImage : register(t0);
Texture2D<float>  SelectionRing   : register(t1);
Texture2D<float>  NoJitterDepth   : register(t2);

SamplerState PointSampler  : register(s0);
SamplerState LinearSampler : register(s1);

ConstantBuffer<FCamera> CameraBuffer : register(b0);

SHADER_CONSTANT_BLOCK_BEGIN
    // 0-16
    int    EnableSelectionOutline;
    int    EnableGrid;
    float  OutlineAlpha;
    float  GridPlaneY;

    // 16-32
    float  GridMinorSize;
    float  GridMajorSize;
    float  GridMinorWidth;
    float  GridMajorWidth;

    // 32-48
    float3 OutlineColor;
    float  GridFadeDistance;

    // 48-64
    float3 GridMinorColor;
    float  GridMinorAlpha;

    // 64-80
    float3 GridMajorColor;
    float  GridMajorAlpha;

    // 80-96
    float  GridHorizonFade;
    float  GridDepthBias;
    float  GridMaxTraceDistance;
    float  Padding2;
SHADER_CONSTANT_BLOCK_END

float GridLinesAA(float2 CoordWS, float SizeWS, float Width)
{
    const float  Size = max(SizeWS, 1e-6);
    const float  W    = max(Width, 1e-3);
    const float2 UV   = CoordWS / Size;
    const float2 FW   = max(fwidth(UV), float2(1e-6, 1e-6));
    const float2 Dist = abs(frac(UV - 0.5) - 0.5);
    const float2 A    = Dist / (FW * W);

    return 1.0 - saturate(min(A.x, A.y));
}

float4 Main(float2 TexCoord : TEXCOORD0, float4 Position : SV_Position) : SV_TARGET0
{
    float3 Color = TonemappedImage.Sample(PointSampler, TexCoord).rgb;

    if (Constants.EnableGrid != 0)
    {
        const float2 NDC         = float2(TexCoord.x * 2.0 - 1.0, (1.0 - TexCoord.y) * 2.0 - 1.0);
        const float3 RayOriginWS = CameraBuffer.PositionWS;
        const float4 ClipFar     = float4(NDC.x, NDC.y, 1.0, 1.0);
        const float4 ViewFar4    = mul(ClipFar, CameraBuffer.ProjectionInvUnjittered);
        const float3 ViewFar     = ViewFar4.xyz / max(ViewFar4.w, 1e-6);
        const float3 RayDirVS    = normalize(ViewFar);
        const float3 RayDirWS    = normalize(mul(float4(RayDirVS, 0.0), CameraBuffer.ViewInv).xyz);

        const float Denom = RayDirWS.y;
        if (abs(Denom) > 1e-6)
        {
            const float PlaneT = (Constants.GridPlaneY - RayOriginWS.y) / Denom;

            // Skip the grid when the plane is behind the camera or past the optional hard trace-distance cut-off.
            const bool WithinTraceDistance = !(Constants.GridMaxTraceDistance > 0.0 && PlaneT > Constants.GridMaxTraceDistance);
            if (PlaneT > 0.0 && WithinTraceDistance)
            {
                float Fade = 1.0;
                if (Constants.GridFadeDistance > 0.0)
                {
                    Fade *= saturate(1.0 - (PlaneT / Constants.GridFadeDistance));
                }

                if (Constants.GridHorizonFade > 0.0)
                {
                    Fade *= saturate(abs(Denom) * Constants.GridHorizonFade);
                }

                if (Fade > 0.0)
                {
                    const uint2 Pixel      = uint2(Position.xy);
                    const float SceneDepth = NoJitterDepth.Load(int3(Pixel, 0)).r;

                    float Visibility = 1.0;
                    if (SceneDepth < 1.0)
                    {
                        const float ViewZ = Depth_ProjToView(SceneDepth, CameraBuffer.ProjectionInvUnjittered);
                        const float RayZ  = RayDirVS.z;

                        if (abs(RayZ) > 1e-6)
                        {
                            const float TScene = ViewZ / RayZ;
                            const float Delta  = TScene - PlaneT;
                            const float Bias   = max(Constants.GridDepthBias, 0.0);
                            const float Soft   = max(Bias * 4.0, abs(fwidth(PlaneT)) * 2.0);

                            Visibility = saturate((Delta - Bias) / max(Soft, 1e-5));
                        }
                    }

                    if (Visibility > 0.0)
                    {
                        const float3 SceneColor    = Color;
                        const float  SceneLuma     = dot(SceneColor, float3(0.2126, 0.7152, 0.0722));
                        const float  ContrastScale = lerp(1.65, 0.55, saturate(SceneLuma));
                        const float  MidGray       = saturate(1.0 - abs(SceneLuma - 0.5) * 2.0);
                        const float  AlphaScale    = 1.0 + (MidGray * 0.35);
                        const float3 MinorColor    = saturate(Constants.GridMinorColor * ContrastScale);
                        const float3 MajorColor    = saturate(Constants.GridMajorColor * ContrastScale);
                        const float3 HitWS         = RayOriginWS + (RayDirWS * PlaneT);
                        const float2 CoordWS       = HitWS.xz;

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

    if (Constants.EnableSelectionOutline != 0)
    {
        const float Ring = SelectionRing.Sample(LinearSampler, TexCoord).r;
        Color = lerp(Color, Constants.OutlineColor, Ring * Constants.OutlineAlpha);
    }

    Color = LinearToSRGB(Color);
    return float4(Color, 1.0);
}

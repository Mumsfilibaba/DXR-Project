#include "Structs.hlsli"
#include "Constants.hlsli"

ConstantBuffer<FCamera>    CameraBuffer    : register(b0);
ConstantBuffer<FTransform> TransformBuffer : register(b1);

struct FVSInput
{
    float3 Position : POSITION0;
};

struct FVSOutput
{
    float4 Position : SV_Position;
};

FVSOutput VSMain(FVSInput Input)
{
    FVSOutput Output = (FVSOutput)0;
    const float3 PositionWS3 = TransformPositionWS(TransformBuffer, Input.Position);
    Output.Position = mul(float4(PositionWS3, 1.0), CameraBuffer.ViewProjection);
    return Output;
}

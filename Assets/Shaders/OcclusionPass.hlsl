#include "Structs.hlsli"
#include "TransformHelpers.hlsli"
#include "Constants.hlsli"

ConstantBuffer<FCamera>    CameraBuffer    : register(b0);
ConstantBuffer<FPerObject> PerObjectBuffer : register(b1);

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
    const float3 PositionWS3 = TransformPositionWS(PerObjectBuffer, Input.Position);
    
    FVSOutput Output = (FVSOutput)0;
    Output.Position = mul(float4(PositionWS3, 1.0), CameraBuffer.ViewProjection);
    return Output;
}

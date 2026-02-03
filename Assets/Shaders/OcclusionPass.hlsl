#include "Structs.hlsli"
#include "Constants.hlsli"

SHADER_CONSTANT_BLOCK_BEGIN
    FTransform Transform;
SHADER_CONSTANT_BLOCK_END

ConstantBuffer<FCamera> CameraBuffer : register(b0);

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
    const float3 PositionWS3 = TransformPositionWS(Constants.Transform, Input.Position);
    Output.Position = mul(float4(PositionWS3, 1.0), CameraBuffer.ViewProjection);
    return Output;
}

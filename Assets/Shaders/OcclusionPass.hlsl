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
    Output.Position = mul(float4(Input.Position, 1.0f), Constants.Transform.Transform);
    Output.Position = mul(Output.Position, CameraBuffer.ViewProjection);
    return Output;
}

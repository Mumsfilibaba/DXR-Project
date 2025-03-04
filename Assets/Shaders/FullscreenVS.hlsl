struct FVSOutput
{
    float2 TexCoord : TEXCOORD0;
    float4 Position : SV_Position;
};

FVSOutput Main(uint VertexID : SV_VertexID)
{
    FVSOutput Output;
    Output.TexCoord   = float2((VertexID << 1) & 2, VertexID & 2);
    Output.Position   = float4((Output.TexCoord * 2.0) - 1.0, 0.0, 1.0);
    Output.Position.y = -Output.Position.y;
    return Output;
}
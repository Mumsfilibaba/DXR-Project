#define COMPRESS_ONE_MIP_THREADGROUP_WIDTH 8
#define COMPRESS_TWO_MIPS_THREADGROUP_WIDTH 16

#define MIP1_BLOCKS_PER_ROW 8

// Constant buffer for Block compression shaders
cbuffer BlockCompressCB : register(b0)
{
    float g_oneOverTextureWidth;
}

// CUSTOMBUILD : warning X4714: sum of temp registers and indexable temp registers times 256 threads exceeds the recommended total 16384.  Performance may be reduced
//  This warning shows up in Debug mode due to the complexity of the unoptimized shaders, but it's harmless aside from the fact that the shaders will be slow in Debug
#pragma warning(disable: 4714)

//--------------------------------------------------------------------------------------
// Name: ColorTo565
// Desc: Pack a 3-component color into a uint
//--------------------------------------------------------------------------------------
uint ColorTo565(float3 Color)
{
    uint3 rgb = round(Color * float3(31.0f, 63.0f, 31.0f));
    return (rgb.r << 11) | (rgb.g << 5) | rgb.b;
}


//--------------------------------------------------------------------------------------
// Name: TexelToUV
// Desc: Convert from a texel to the UV coordinates used in a Gather call
//--------------------------------------------------------------------------------------
float2 TexelToUV(float2 Texel, float OneOverTextureWidth)
{
    // We Gather from the bottom-right corner of the Texel
    return (Texel + 1.0f) * OneOverTextureWidth;
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsRGB
// Desc: Load the 16 RGB texels that form a Block
//--------------------------------------------------------------------------------------
void LoadTexelsRGB(Texture2D Tex, SamplerState Samp, float OneOverTextureWidth, uint2 ThreadIDWithinDispatch, out float3 Block[16])
{
    float2 TexCoord = TexelToUV(float2(ThreadIDWithinDispatch * 4), OneOverTextureWidth);

    float4 Red   = Tex.GatherRed(Samp, TexCoord, int2(0, 0));
    float4 Green = Tex.GatherGreen(Samp, TexCoord, int2(0, 0));
    float4 Blue  = Tex.GatherBlue(Samp, TexCoord, int2(0, 0));
    Block[0] = float3(Red[3], Green[3], Blue[3]);
    Block[1] = float3(Red[2], Green[2], Blue[2]);
    Block[4] = float3(Red[0], Green[0], Blue[0]);
    Block[5] = float3(Red[1], Green[1], Blue[1]);

    Red   = Tex.GatherRed(Samp, TexCoord, int2(2, 0));
    Green = Tex.GatherGreen(Samp, TexCoord, int2(2, 0));
    Blue  = Tex.GatherBlue(Samp, TexCoord, int2(2, 0));
    Block[2] = float3(Red[3], Green[3], Blue[3]);
    Block[3] = float3(Red[2], Green[2], Blue[2]);
    Block[6] = float3(Red[0], Green[0], Blue[0]);
    Block[7] = float3(Red[1], Green[1], Blue[1]);

    Red   = Tex.GatherRed(Samp, TexCoord, int2(0, 2));
    Green = Tex.GatherGreen(Samp, TexCoord, int2(0, 2));
    Blue  = Tex.GatherBlue(Samp, TexCoord, int2(0, 2));
    Block[8] = float3(Red[3], Green[3], Blue[3]);
    Block[9] = float3(Red[2], Green[2], Blue[2]);
    Block[12] = float3(Red[0], Green[0], Blue[0]);
    Block[13] = float3(Red[1], Green[1], Blue[1]);

    Red   = Tex.GatherRed(Samp, TexCoord, int2(2, 2));
    Green = Tex.GatherGreen(Samp, TexCoord, int2(2, 2));
    Blue  = Tex.GatherBlue(Samp, TexCoord, int2(2, 2));
    Block[10] = float3(Red[3], Green[3], Blue[3]);
    Block[11] = float3(Red[2], Green[2], Blue[2]);
    Block[14] = float3(Red[0], Green[0], Blue[0]);
    Block[15] = float3(Red[1], Green[1], Blue[1]);
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsRGBBias
// Desc: Load the 16 RGB texels that form a Block, with a mip bias
//--------------------------------------------------------------------------------------
void LoadTexelsRGBBias(Texture2D Tex, SamplerState Samp, float OneOverTextureSize, uint2 ThreadIDWithinDispatch, uint MipBias, out float3 Block[16])
{
    // We need to use Sample rather than Gather/Load for the Bias functions, because low mips will read outside
    //  the texture boundary. When reading outside the boundary, Gather/Load return 0, but Sample can clamp
    float2 Location = float2(ThreadIDWithinDispatch * 4) * OneOverTextureSize;
    Block[0]  = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 0)).rgb;
    Block[1]  = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 0)).rgb;
    Block[2]  = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 0)).rgb;
    Block[3]  = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 0)).rgb;
    Block[4]  = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 1)).rgb;
    Block[5]  = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 1)).rgb;
    Block[6]  = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 1)).rgb;
    Block[7]  = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 1)).rgb;
    Block[8]  = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 2)).rgb;
    Block[9]  = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 2)).rgb;
    Block[10] = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 2)).rgb;
    Block[11] = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 2)).rgb;
    Block[12] = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 3)).rgb;
    Block[13] = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 3)).rgb;
    Block[14] = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 3)).rgb;
    Block[15] = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 3)).rgb;
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsRGBA
// Desc: Load the 16 RGBA texels that form a Block
//--------------------------------------------------------------------------------------
void LoadTexelsRGBA(Texture2D Tex, uint2 ThreadIDWithinDispatch, out float3 BlockRGB[16], out float BlockA[16])
{
    float4 rgba;
    int3 Location = int3(ThreadIDWithinDispatch * 4, 0);
    rgba = Tex.Load(Location, int2(0, 0)); BlockRGB[0]  = rgba.rgb; BlockA[0]  = rgba.a;
    rgba = Tex.Load(Location, int2(1, 0)); BlockRGB[1]  = rgba.rgb; BlockA[1]  = rgba.a;
    rgba = Tex.Load(Location, int2(2, 0)); BlockRGB[2]  = rgba.rgb; BlockA[2]  = rgba.a;
    rgba = Tex.Load(Location, int2(3, 0)); BlockRGB[3]  = rgba.rgb; BlockA[3]  = rgba.a;
    rgba = Tex.Load(Location, int2(0, 1)); BlockRGB[4]  = rgba.rgb; BlockA[4]  = rgba.a;
    rgba = Tex.Load(Location, int2(1, 1)); BlockRGB[5]  = rgba.rgb; BlockA[5]  = rgba.a;
    rgba = Tex.Load(Location, int2(2, 1)); BlockRGB[6]  = rgba.rgb; BlockA[6]  = rgba.a;
    rgba = Tex.Load(Location, int2(3, 1)); BlockRGB[7]  = rgba.rgb; BlockA[7]  = rgba.a;
    rgba = Tex.Load(Location, int2(0, 2)); BlockRGB[8]  = rgba.rgb; BlockA[8]  = rgba.a;
    rgba = Tex.Load(Location, int2(1, 2)); BlockRGB[9]  = rgba.rgb; BlockA[9]  = rgba.a;
    rgba = Tex.Load(Location, int2(2, 2)); BlockRGB[10] = rgba.rgb; BlockA[10] = rgba.a;
    rgba = Tex.Load(Location, int2(3, 2)); BlockRGB[11] = rgba.rgb; BlockA[11] = rgba.a;
    rgba = Tex.Load(Location, int2(0, 3)); BlockRGB[12] = rgba.rgb; BlockA[12] = rgba.a;
    rgba = Tex.Load(Location, int2(1, 3)); BlockRGB[13] = rgba.rgb; BlockA[13] = rgba.a;
    rgba = Tex.Load(Location, int2(2, 3)); BlockRGB[14] = rgba.rgb; BlockA[14] = rgba.a;
    rgba = Tex.Load(Location, int2(3, 3)); BlockRGB[15] = rgba.rgb; BlockA[15] = rgba.a;
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsRGBABias
// Desc: Load the 16 RGBA texels that form a Block, with a mip bias
//--------------------------------------------------------------------------------------
void LoadTexelsRGBABias(Texture2D Tex, SamplerState Samp, float OneOverTextureSize, uint2 ThreadIDWithinDispatch, uint MipBias, out float3 BlockRGB[16], out float BlockA[16])
{
    // We need to use Sample rather than Gather/Load for the Bias functions, because low mips will read outside
    //  the texture boundary. When reading outside the boundary, Gather/Load return 0, but Sample will clamp
    float4 rgba;
    float2 Location = float2(ThreadIDWithinDispatch * 4) * OneOverTextureSize;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 0)); BlockRGB[0]  = rgba.rgb; BlockA[0]  = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 0)); BlockRGB[1]  = rgba.rgb; BlockA[1]  = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 0)); BlockRGB[2]  = rgba.rgb; BlockA[2]  = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 0)); BlockRGB[3]  = rgba.rgb; BlockA[3]  = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 1)); BlockRGB[4]  = rgba.rgb; BlockA[4]  = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 1)); BlockRGB[5]  = rgba.rgb; BlockA[5]  = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 1)); BlockRGB[6]  = rgba.rgb; BlockA[6]  = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 1)); BlockRGB[7]  = rgba.rgb; BlockA[7]  = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 2)); BlockRGB[8]  = rgba.rgb; BlockA[8]  = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 2)); BlockRGB[9]  = rgba.rgb; BlockA[9]  = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 2)); BlockRGB[10] = rgba.rgb; BlockA[10] = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 2)); BlockRGB[11] = rgba.rgb; BlockA[11] = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 3)); BlockRGB[12] = rgba.rgb; BlockA[12] = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 3)); BlockRGB[13] = rgba.rgb; BlockA[13] = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 3)); BlockRGB[14] = rgba.rgb; BlockA[14] = rgba.a;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 3)); BlockRGB[15] = rgba.rgb; BlockA[15] = rgba.a;
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsUV
// Desc: Load the 16 UV texels that form a Block
//--------------------------------------------------------------------------------------
void LoadTexelsUV(Texture2D Tex, SamplerState Samp, float OneOverTextureWidth, uint2 ThreadIDWithinDispatch, out float BlockU[16], out float BlockV[16])
{
    float2 TexCoord = TexelToUV(float2(ThreadIDWithinDispatch * 4), OneOverTextureWidth);

    float4 Red   = Tex.GatherRed(Samp, TexCoord, int2(0, 0));
    float4 Green = Tex.GatherGreen(Samp, TexCoord, int2(0, 0));
    BlockU[0] = Red[3]; BlockV[0] = Green[3];
    BlockU[1] = Red[2]; BlockV[1] = Green[2];
    BlockU[4] = Red[0]; BlockV[4] = Green[0];
    BlockU[5] = Red[1]; BlockV[5] = Green[1];

    Red   = Tex.GatherRed(Samp, TexCoord, int2(2, 0));
    Green = Tex.GatherGreen(Samp, TexCoord, int2(2, 0));
    BlockU[2] = Red[3]; BlockV[2] = Green[3];
    BlockU[3] = Red[2]; BlockV[3] = Green[2];
    BlockU[6] = Red[0]; BlockV[6] = Green[0];
    BlockU[7] = Red[1]; BlockV[7] = Green[1];

    Red   = Tex.GatherRed(Samp, TexCoord, int2(0, 2));
    Green = Tex.GatherGreen(Samp, TexCoord, int2(0, 2));
    BlockU[8] = Red[3]; BlockV[8] = Green[3];
    BlockU[9] = Red[2]; BlockV[9] = Green[2];
    BlockU[12] = Red[0]; BlockV[12] = Green[0];
    BlockU[13] = Red[1]; BlockV[13] = Green[1];

    Red   = Tex.GatherRed(Samp, TexCoord, int2(2, 2));
    Green = Tex.GatherGreen(Samp, TexCoord, int2(2, 2));
    BlockU[10] = Red[3]; BlockV[10] = Green[3];
    BlockU[11] = Red[2]; BlockV[11] = Green[2];
    BlockU[14] = Red[0]; BlockV[14] = Green[0];
    BlockU[15] = Red[1]; BlockV[15] = Green[1];
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsUVBias
// Desc: Load the 16 UV texels that form a Block, with a mip bias
//--------------------------------------------------------------------------------------
void LoadTexelsUVBias(Texture2D Tex, SamplerState Samp, float OneOverTextureSize, uint2 ThreadIDWithinDispatch, uint MipBias, out float BlockU[16], out float BlockV[16])
{
    // We need to use Sample rather than Gather/Load for the Bias functions, because low mips will read outside
    //  the texture boundary. When reading outside the boundary, Gather/Load return 0, but Sample will clamp
    float4 rgba;
    float2 Location = float2(ThreadIDWithinDispatch * 4) * OneOverTextureSize;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 0)); BlockU[0]  = rgba.r; BlockV[0]  = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 0)); BlockU[1]  = rgba.r; BlockV[1]  = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 0)); BlockU[2]  = rgba.r; BlockV[2]  = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 0)); BlockU[3]  = rgba.r; BlockV[3]  = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 1)); BlockU[4]  = rgba.r; BlockV[4]  = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 1)); BlockU[5]  = rgba.r; BlockV[5]  = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 1)); BlockU[6]  = rgba.r; BlockV[6]  = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 1)); BlockU[7]  = rgba.r; BlockV[7]  = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 2)); BlockU[8]  = rgba.r; BlockV[8]  = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 2)); BlockU[9]  = rgba.r; BlockV[9]  = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 2)); BlockU[10] = rgba.r; BlockV[10] = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 2)); BlockU[11] = rgba.r; BlockV[11] = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(0, 3)); BlockU[12] = rgba.r; BlockV[12] = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(1, 3)); BlockU[13] = rgba.r; BlockV[13] = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(2, 3)); BlockU[14] = rgba.r; BlockV[14] = rgba.g;
    rgba = Tex.SampleLevel(Samp, Location, MipBias, int2(3, 3)); BlockU[15] = rgba.r; BlockV[15] = rgba.g;
}


//--------------------------------------------------------------------------------------
// Name: GetMinMaxChannel
// Desc: Get the min and max of a single channel
//--------------------------------------------------------------------------------------
void GetMinMaxChannel(float Block[16], out float MinC, out float MaxC)
{
    MinC = Block[0];
    MaxC = Block[0];

    for (int i = 1; i < 16; ++i)
    {
        MinC = min(MinC, Block[i]);
        MaxC = max(MaxC, Block[i]);
    }
}


//--------------------------------------------------------------------------------------
// Name: GetMinMaxUV
// Desc: Get the min and max of two channels (UV)
//--------------------------------------------------------------------------------------
void GetMinMaxUV(float BlockU[16], float BlockV[16], out float MinU, out float MaxU, out float MinV, out float MaxV)
{
    MinU = BlockU[0];
    MaxU = BlockU[0];
    MinV = BlockV[0];
    MaxV = BlockV[0];

    for (int i = 1; i < 16; ++i)
    {
        MinU = min(MinU, BlockU[i]);
        MaxU = max(MaxU, BlockU[i]);
        MinV = min(MinV, BlockV[i]);
        MaxV = max(MaxV, BlockV[i]);
    }
}


//--------------------------------------------------------------------------------------
// Name: GetMinMaxRGB
// Desc: Get the min and max of three channels (RGB)
//--------------------------------------------------------------------------------------
void GetMinMaxRGB(float3 ColorBlock[16], out float3 MinColor, out float3 MaxColor)
{
    MinColor = ColorBlock[0];
    MaxColor = ColorBlock[0];

    for (int i = 1; i < 16; ++i)
    {
        MinColor = min(MinColor, ColorBlock[i]);
        MaxColor = max(MaxColor, ColorBlock[i]);
    }
}


//--------------------------------------------------------------------------------------
// Name: InsetMinMaxRGB
// Desc: Slightly inset the min and max color values to reduce RMS error.
//      This is recommended by van Waveren & Castano, "Real-Time YCoCg-DXT Compression"
//      http://www.nvidia.com/object/real-time-ycocg-dxt-compression.html
//--------------------------------------------------------------------------------------
void InsetMinMaxRGB(inout float3 MinColor, inout float3 MaxColor, float ColorScale)
{
    // Since we have four points, (1/16) * (max-min) will give us half the distance between
    //  two points on the line in color space
    float3 Offset = (1.0f / 16.0f) * (MaxColor - MinColor);

    // After applying the Offset, we want to round up or down to the next integral color value (0 to 255)
    ColorScale *= 255.0f;
    MaxColor = ceil((MaxColor - Offset) * ColorScale) / ColorScale;
    MinColor = floor((MinColor + Offset) * ColorScale) / ColorScale;
}


//--------------------------------------------------------------------------------------
// Name: GetIndicesRGB
// Desc: Calculate the BC block indices for each Color in the block
//--------------------------------------------------------------------------------------
uint GetIndicesRGB(float3 Block[16], float3 MinColor, float3 MaxColor)
{
    uint Indices = 0;

    // For each input Color, we need to select between one of the following output colors:
    //  0: MaxColor
    //  1: (2/3)*MaxColor + (1/3)*MinColor
    //  2: (1/3)*MaxColor + (2/3)*MinColor
    //  3: MinColor  
    //
    // We essentially just project (Block[i] - MaxColor) onto (MinColor - MaxColor), but we pull out
    //  a few constant terms.
    float3 diag = MinColor - MaxColor;
    float stepInc = 3.0f / dot(diag, diag); // Scale up by 3, because our indices are between 0 and 3
    diag *= stepInc;
    float c = stepInc * (dot(MaxColor, MaxColor) - dot(MaxColor, MinColor));

    for (int i = 15; i >= 0; --i)
    {
        // Compute the index for this Block element
        uint index = round(dot(Block[i], diag) + c);

        // Now we need to convert our index into the somewhat unintuivive BC1 indexing scheme:
        //  0: MaxColor
        //  1: MinColor
        //  2: (2/3)*MaxColor + (1/3)*MinColor
        //  3: (1/3)*MaxColor + (2/3)*MinColor
        //
        // The mapping is:
        //  0 -> 0
        //  1 -> 2
        //  2 -> 3
        //  3 -> 1
        //
        // We can perform this mapping using bitwise operations, which is faster
        //  than predication or branching as long as it doesn't increase our register
        //  count too much. The mapping in binary looks like:
        //  00 -> 00
        //  01 -> 10
        //  10 -> 11
        //  11 -> 01
        //
        // Splitting it up by bit, the output looks like:
        //  bit1_out = bit0_in XOR bit1_in
        //  bit0_out = bit1_in 
        uint bit0_in = index & 1;
        uint bit1_in = index >> 1;
        Indices |= ((bit0_in^bit1_in) << 1) | bit1_in;

        if (i != 0)
        {
            Indices <<= 2;
        }
    }

    return Indices;
}


//--------------------------------------------------------------------------------------
// Name: GetIndicesAlpha
// Desc: Calculate the BC Block Indices for an alpha channel
//--------------------------------------------------------------------------------------
void GetIndicesAlpha(float Block[16], float minA, float maxA, inout uint2 packed)
{
    float d = minA - maxA;
    float stepInc = 7.0f / d;

    // Both packed.x and packed.y contain index values, so we need two loops

    uint index = 0;
    uint shift = 16;
    for (int i = 0; i < 6; ++i)
    {
        // For each input alpha value, we need to select between one of eight output values
        //  0: maxA
        //  1: (6/7)*maxA + (1/7)*minA
        //  ...
        //  6: (1/7)*maxA + (6/3)*minA
        //  7: minA  
        index = round(stepInc * (Block[i] - maxA));

        // Now we need to convert our index into the BC indexing scheme:
        //  0: maxA
        //  1: minA
        //  2: (6/7)*maxA + (1/7)*minA
        //  ...
        //  7: (1/7)*maxA + (6/3)*minA
        index += (index > 0) - (7 * (index == 7));

        packed.x |= (index << shift);
        shift += 3;
    }

    // The 6th index straddles the two uints
    packed.y |= (index >> 1);

    shift = 2;
    for (i = 6; i < 16; ++i)
    {
        index = round((Block[i] - maxA) * stepInc);
        index += (index > 0) - (7 * (index == 7));

        packed.y |= (index << shift);
        shift += 3;
    }
}


//--------------------------------------------------------------------------------------
// Name: CompressBC1Block
// Desc: Compress a BC1 Block. ColorScale is a scale value to be applied to the input 
//          colors; this used as an optimization when compressing two mips at a time.
//          When compressing only a single mip, ColorScale is always 1.0
//--------------------------------------------------------------------------------------
uint2 CompressBC1Block(float3 Block[16], float ColorScale = 1.0f)
{
    float3 MinColor, MaxColor;
    GetMinMaxRGB(Block, MinColor, MaxColor);

    // Inset the min and max values
    InsetMinMaxRGB(MinColor, MaxColor, ColorScale);

    // Pack our colors into uints
    uint minColor565 = ColorTo565(ColorScale * MinColor);
    uint maxColor565 = ColorTo565(ColorScale * MaxColor);

    uint Indices = 0;
    if (minColor565 < maxColor565)
    {
        Indices = GetIndicesRGB(Block, MinColor, MaxColor);
    }

    return uint2((minColor565 << 16) | maxColor565, Indices);
}


//--------------------------------------------------------------------------------------
// Name: CompressBC3Block
// Desc: Compress a BC3 Block. ValueScale is a scale value to be applied to the input 
//          values; this used as an optimization when compressing two mips at a time.
//          When compressing only a single mip, ValueScale is always 1.0
//--------------------------------------------------------------------------------------
uint4 CompressBC3Block(float3 BlockRGB[16], float BlockA[16], float ValueScale = 1.0f)
{
    float3 MinColor, MaxColor;
    float minA, maxA;
    GetMinMaxRGB(BlockRGB, MinColor, MaxColor);
    GetMinMaxChannel(BlockA, minA, maxA);

    // Inset the min and max Color values. We don't inset the alpha values
    //  because, while it may reduce the RMS error, it has a tendency to turn
    //  fully opaque texels partially transparent, which is probably not desirable.
    InsetMinMaxRGB(MinColor, MaxColor, ValueScale);

    // Pack our colors and alpha values into uints
    uint minColor565 = ColorTo565(ValueScale * MinColor);
    uint maxColor565 = ColorTo565(ValueScale * MaxColor);
    uint minAPacked = round(minA * ValueScale * 255.0f);
    uint maxAPacked = round(maxA * ValueScale * 255.0f);

    uint Indices = 0;
    if (minColor565 < maxColor565)
    {
        Indices = GetIndicesRGB(BlockRGB, MinColor, MaxColor);
    }

    uint2 outA = uint2((minAPacked << 8) | maxAPacked, 0);
    if (minAPacked < maxAPacked)
    {
        GetIndicesAlpha(BlockA, minA, maxA, outA);
    }

    return uint4(outA.x, outA.y, (minColor565 << 16) | maxColor565, Indices);
}


//--------------------------------------------------------------------------------------
// Name: CompressBC5Block
// Desc: Compress a BC5 Block. ValueScale is a scale value to be applied to the input 
//          values; this used as an optimization when compressing two mips at a time.
//          When compressing only a single mip, ValueScale is always 1.0
//--------------------------------------------------------------------------------------
uint4 CompressBC5Block(float BlockU[16], float BlockV[16], float ValueScale = 1.0f)
{
    float MinU, MaxU, MinV, MaxV;
    GetMinMaxUV(BlockU, BlockV, MinU, MaxU, MinV, MaxV);

    // Pack our min and max TexCoord values
    uint minUPacked = round(MinU * ValueScale * 255.0f);
    uint maxUPacked = round(MaxU * ValueScale * 255.0f);
    uint minVPacked = round(MinV * ValueScale * 255.0f);
    uint maxVPacked = round(MaxV * ValueScale * 255.0f);

    uint2 outU = uint2((minUPacked << 8) | maxUPacked, 0);
    uint2 outV = uint2((minVPacked << 8) | maxVPacked, 0);

    if (minUPacked < maxUPacked)BlockID
    {
        GetIndicesAlpha(BlockU, MinU, MaxU, outU);
    }

    if (minVPacked < maxVPacked)
    {
        GetIndicesAlpha(BlockV, MinV, MaxV, outV);
    }

    return uint4(outU.x, outU.y, outV.x, outV.y);
}


//--------------------------------------------------------------------------------------
// Name: CalcTailMipsParams
// Desc: Calculate parameters used in the "compress tail mips" shaders
//--------------------------------------------------------------------------------------
void CalcTailMipsParams(uint2 ThreadIDWithinDispatch, out float OneOverTextureSize, out uint2 BlockID, out uint MipBias)
{
    BlockID = ThreadIDWithinDispatch;
    MipBias = 0;
    OneOverTextureSize = 1;

    // When compressing our tail mips, we only dispatch one 8x8 threadgroup. Different threads
    //  are selected to compress different mip levels based on the position of thr thread in
    //  the threadgroup.
    if (BlockID.x < 4)
    {
        if (BlockID.y < 4)
        {
            // 16x16 mip
            OneOverTextureSize = 1.0f / 16.0f;
        }
        else
        {
            // 1x1 mip
            MipBias = 4;
            BlockID.y -= 4;
        }
    }
    else if (BlockID.x < 6)
    {
        // 8x8 mip
        MipBias = 1;
        BlockID -= float2(4, 4);
        OneOverTextureSize = 1.0f / 8.0f;
    }
    else if (BlockID.x < 7)
    {
        // 4x4 mip
        MipBias = 2;
        BlockID -= float2(6, 6);
        OneOverTextureSize = 1.0f / 4.0f;
    }
    else if (BlockID.x < 8)
    {
        // 2x2 mip
        MipBias = 3;
        BlockID -= float2(7, 7);
        OneOverTextureSize = 1.0f / 2.0f;
    }
}
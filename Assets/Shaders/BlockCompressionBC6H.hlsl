#include "CoreDefines.hlsli"

// Modified version of: https://github.com/knarkowicz/GPURealTimeBC6H/blob/master/bin/compress.hlsl

// "loop control variable conflicts with A previous declaration in the outer scope"
#pragma warning(disable : 3078)

// Whether to use P2 modes (4 endpoints) for compression. Slow, but improves quality.
#ifdef ENABLE_CUBE_MAP
	#define QUALITY (0)
#else
	#define QUALITY (1)
#endif

#define ENCODE_P2 (QUALITY == 1)

// Improve quality at small performance loss
#define INSET_COLOR_BBOX (1)
#define OPTIMIZE_ENDPOINTS (1)

// Whether to optimize for luminance error or for RGB error
#define LUMINANCE_WEIGHTS (1)

#ifndef NUM_THREADS
	#define NUM_THREADS (8)
#endif

static const float HALF_MAX    = 65504.0;
static const uint  PATTERN_NUM = 32;

#ifdef ENABLE_CUBE_MAP
TextureCube<float4>     SourceTexture : register(t0);
RWTexture2DArray<uint4> OutputTexture : register(u0);
#else
Texture2D<float4>  SourceTexture : register(t0);
RWTexture2D<uint4> OutputTexture : register(u0);
#endif

float3 TexCoordToCubeMapDir(in float2 TexCoord, in uint FaceIndex)
{
	// Convert TexCoord into [-1, 1] range:
	TexCoord = TexCoord * 2.0 - 1.0;

	// and UV.y should point upwards:
	TexCoord.y *= -1.0;

	switch (FaceIndex)
	{
	case 0:
		// +X
		return normalize(float3(1.0, TexCoord.y, -TexCoord.x));
	case 1:
		// -X
		return normalize(float3(-1.0, TexCoord.yx));
	case 2:
		// +Y
		return normalize(float3(TexCoord.x, 1.0, -TexCoord.y));
	case 3:
		// -Y
		return normalize(float3(TexCoord.x, -1.0, TexCoord.y));
	case 4:
		// +Z
		return normalize(float3(TexCoord, 1.0));
	case 5:
		// -Z
		return normalize(float3(-TexCoord.x, TexCoord.y, -1.0));
	default:
		// error
		return 0;
	}
}

// TODO: This could def. be a static sampler
SamplerState PointSampler : register(s0);

SHADER_CONSTANT_BLOCK_BEGIN
	uint2  TextureSizeInBlocks;
	float2 TextureSizeRcp;
SHADER_CONSTANT_BLOCK_END

float CalcMSLE(float3 A, float3 B)
{
	float3 Delta   = log2((B + 1.0) / (A + 1.0));
	float3 DeltaSq = Delta * Delta;

#if LUMINANCE_WEIGHTS
	float3 LuminanceWeights = float3(0.299, 0.587, 0.114);
	DeltaSq *= LuminanceWeights;
#endif

	return DeltaSq.x + DeltaSq.y + DeltaSq.z;
}

uint PatternFixupID(uint i)
{
	uint Ret = 15;
	Ret = ((uint(3441033216) >> i) & 0x1) ? 2 : Ret;
	Ret = ((uint(845414400)  >> i) & 0x1) ? 8 : Ret;
	return Ret;
}

uint Pattern(uint PatternIndex, uint i)
{
	uint P2 = PatternIndex / 2;
	uint P3 = PatternIndex - P2 * 2;

	uint Enc = 0;
	Enc = P2 == 0  ? 2290666700 : Enc;
	Enc = P2 == 1  ? 3972591342 : Enc;
	Enc = P2 == 2  ? 4276930688 : Enc;
	Enc = P2 == 3  ? 3967876808 : Enc;
	Enc = P2 == 4  ? 4293707776 : Enc;
	Enc = P2 == 5  ? 3892379264 : Enc;
	Enc = P2 == 6  ? 4278255592 : Enc;
	Enc = P2 == 7  ? 4026597360 : Enc;
	Enc = P2 == 8  ? 9369360    : Enc;
	Enc = P2 == 9  ? 147747072  : Enc;
	Enc = P2 == 10 ? 1930428556 : Enc;
	Enc = P2 == 11 ? 2362323200 : Enc;
	Enc = P2 == 12 ? 823134348  : Enc;
	Enc = P2 == 13 ? 913073766  : Enc;
	Enc = P2 == 14 ? 267393000  : Enc;
	Enc = P2 == 15 ? 966553998  : Enc;

	Enc = P3 ? Enc >> 16 : Enc;
	uint Ret = (Enc >> i) & 0x1;
	return Ret;
}

float3 Quantize7(float3 x)
{
	return (f32tof16(x) * 128.0) / (0x7bff + 1.0);
}

float3 Quantize9(float3 x)
{
	return (f32tof16(x) * 512.0) / (0x7bff + 1.0);
}

float3 Quantize10(float3 x)
{
	return (f32tof16(x) * 1024.0) / (0x7bff + 1.0);
}

float3 Unquantize7(float3 x)
{
	return (x * 65536.0 + 0x8000) / 128.0;
}

float3 Unquantize9(float3 x)
{
	return (x * 65536.0 + 0x8000) / 512.0;
}

float3 Unquantize10(float3 x)
{
	return (x * 65536.0 + 0x8000) / 1024.0;
}

float3 FinishUnquantize(float3 Endpoint0Unq, float3 Endpoint1Unq, float Weight)
{
	float3 Comp = (Endpoint0Unq * (64.0 - Weight) + Endpoint1Unq * Weight + 32.0) * (31.0 / 4096.0);
	return f16tof32(uint3(Comp));
}

void Swap(inout float3 A, inout float3 B)
{
	float3 Temp = A;
	A = B;
	B = Temp;
}

void Swap(inout float A, inout float B)
{
	float Temp = A;
	A = B;
	B = Temp;
}

uint ComputeIndex3(float TexelPos, float EndPoint0Pos, float EndPoint1Pos)
{
	float Res = (TexelPos - EndPoint0Pos) / (EndPoint1Pos - EndPoint0Pos);
	return (uint)clamp(Res * 6.98182 + 0.00909 + 0.5, 0.0, 7.0);
}

uint ComputeIndex4(float TexelPos, float EndPoint0Pos, float EndPoint1Pos)
{
	float Res = (TexelPos - EndPoint0Pos) / (EndPoint1Pos - EndPoint0Pos);
	return (uint)clamp(Res * 14.93333 + 0.03333 + 0.5, 0.0, 15.0);
}

void SignExtend(inout float3 V1, uint Mask, uint SignFlag)
{
	int3 V = (int3) V1;
	V.x = (V.x & Mask) | (V.x < 0 ? SignFlag : 0);
	V.y = (V.y & Mask) | (V.y < 0 ? SignFlag : 0);
	V.z = (V.z & Mask) | (V.z < 0 ? SignFlag : 0);
	V1 = V;
}

// Refine endpoints by insetting bounding box in log2 RGB space
void InsetColorBBoxP1(float3 Texels[16], inout float3 BlockMin, inout float3 BlockMax)
{
	float3 RefinedBlockMin = BlockMax;
	float3 RefinedBlockMax = BlockMin;

	for (uint i = 0; i < 16; ++i)
	{
		RefinedBlockMin = min(RefinedBlockMin, all(Texels[i] == BlockMin) ? RefinedBlockMin : Texels[i]);
		RefinedBlockMax = max(RefinedBlockMax, all(Texels[i] == BlockMax) ? RefinedBlockMax : Texels[i]);
	}

	float3 LogRefinedBlockMax = log2(RefinedBlockMax + 1.0);
	float3 LogRefinedBlockMin = log2(RefinedBlockMin + 1.0);

	float3 LogBlockMax    = log2(BlockMax + 1.0);
	float3 LogBlockMin    = log2(BlockMin + 1.0);
	float3 LogBlockMaxExt = (LogBlockMax - LogBlockMin) * (1.0 / 32.0);

	LogBlockMin += min(LogRefinedBlockMin - LogBlockMin, LogBlockMaxExt);
	LogBlockMax -= min(LogBlockMax - LogRefinedBlockMax, LogBlockMaxExt);

	BlockMin = exp2(LogBlockMin) - 1.0;
	BlockMax = exp2(LogBlockMax) - 1.0;
}

// Refine endpoints by insetting bounding box in log2 RGB space
void InsetColorBBoxP2(float3 Texels[16], uint PatternIndex, uint PatternSelector, inout float3 BlockMin, inout float3 BlockMax)
{
	float3 RefinedBlockMin = BlockMax;
	float3 RefinedBlockMax = BlockMin;

	for (uint i = 0; i < 16; ++i)
	{
		uint PaletteID = Pattern(PatternIndex, i);
		if (PaletteID == PatternSelector)
		{
			RefinedBlockMin = min(RefinedBlockMin, all(Texels[i] == BlockMin) ? RefinedBlockMin : Texels[i]);
			RefinedBlockMax = max(RefinedBlockMax, all(Texels[i] == BlockMax) ? RefinedBlockMax : Texels[i]);
		}
	}

	float3 LogRefinedBlockMax = log2(RefinedBlockMax + 1.0);
	float3 LogRefinedBlockMin = log2(RefinedBlockMin + 1.0);

	float3 LogBlockMax    = log2(BlockMax + 1.0);
	float3 LogBlockMin    = log2(BlockMin + 1.0);
	float3 LogBlockMaxExt = (LogBlockMax - LogBlockMin) * (1.0 / 32.0);

	LogBlockMin += min(LogRefinedBlockMin - LogBlockMin, LogBlockMaxExt);
	LogBlockMax -= min(LogBlockMax - LogRefinedBlockMax, LogBlockMaxExt);

	BlockMin = exp2(LogBlockMin) - 1.0;
	BlockMax = exp2(LogBlockMax) - 1.0;
}

// Least squares optimization to find best endpoints for the selected block indices
void OptimizeEndpointsP1(float3 Texels[16], inout float3 BlockMin, inout float3 BlockMax, in float3 BlockMinNonInset, in float3 BlockMaxNonInset)
{
	float3 BlockDir = BlockMax - BlockMin;
	BlockDir = BlockDir / (BlockDir.x + BlockDir.y + BlockDir.z);

	float EndPoint0Pos = f32tof16(dot(BlockMin, BlockDir));
	float EndPoint1Pos = f32tof16(dot(BlockMax, BlockDir));

	float3 AlphaTexelSum = 0.0;
	float3 BetaTexelSum  = 0.0;
	float  AlphaBetaSum  = 0.0;
	float  AlphaSqSum    = 0.0;
	float  BetaSqSum     = 0.0;

	for (int i = 0; i < 16; i++)
	{
		float TexelPos   = f32tof16(dot(Texels[i], BlockDir));
		uint  TexelIndex = ComputeIndex4(TexelPos, EndPoint0Pos, EndPoint1Pos);

		float Beta  = saturate(TexelIndex / 15.0);
		float Alpha = 1.0 - Beta;

		float3 TexelF16 = f32tof16(Texels[i].xyz);
		AlphaTexelSum += Alpha * TexelF16;
		BetaTexelSum  += Beta  * TexelF16;

		AlphaBetaSum += Alpha * Beta;

		AlphaSqSum += Alpha * Alpha;
		BetaSqSum  += Beta  * Beta;
	}

	float Det = AlphaSqSum * BetaSqSum - AlphaBetaSum * AlphaBetaSum;
	if (abs(Det) > 0.00001f)
	{
		float DetRcp = rcp(Det);
		BlockMin = clamp(f16tof32(clamp(DetRcp * (AlphaTexelSum * BetaSqSum - BetaTexelSum * AlphaBetaSum), 0.0, HALF_MAX)), BlockMinNonInset, BlockMaxNonInset);
		BlockMax = clamp(f16tof32(clamp(DetRcp * (BetaTexelSum * AlphaSqSum - AlphaTexelSum * AlphaBetaSum), 0.0, HALF_MAX)), BlockMinNonInset, BlockMaxNonInset);
	}
}

// Least squares optimization to find best endpoints for the selected block indices
void OptimizeEndpointsP2(float3 Texels[16], uint PatternIndex, uint PatternSelector, inout float3 BlockMin, inout float3 BlockMax)
{
	float3 BlockDir = BlockMax - BlockMin;
	BlockDir = BlockDir / (BlockDir.x + BlockDir.y + BlockDir.z);

	float EndPoint0Pos = f32tof16(dot(BlockMin, BlockDir));
	float EndPoint1Pos = f32tof16(dot(BlockMax, BlockDir));

	float3 AlphaTexelSum = 0.0;
	float3 BetaTexelSum  = 0.0;
	float  AlphaBetaSum  = 0.0;
	float  AlphaSqSum    = 0.0;
	float  BetaSqSum     = 0.0;

	for (int i = 0; i < 16; i++)
	{
		uint PaletteID = Pattern(PatternIndex, i);
		if (PaletteID == PatternSelector)
		{
			float TexelPos   = f32tof16(dot(Texels[i], BlockDir));
			uint  TexelIndex = ComputeIndex3(TexelPos, EndPoint0Pos, EndPoint1Pos);

			float Beta  = saturate(TexelIndex / 7.0);
			float Alpha = 1.0 - Beta;

			float3 TexelF16 = f32tof16(Texels[i].xyz);
			AlphaTexelSum += Alpha * TexelF16;
			BetaTexelSum  += Beta * TexelF16;

			AlphaBetaSum += Alpha * Beta;

			AlphaSqSum += Alpha * Alpha;
			BetaSqSum  += Beta * Beta;
		}
	}

	float Det = AlphaSqSum * BetaSqSum - AlphaBetaSum * AlphaBetaSum;
	if (abs(Det) > 0.00001f)
	{
		float DetRcp = rcp(Det);
		BlockMin = f16tof32(clamp(DetRcp * (AlphaTexelSum * BetaSqSum - BetaTexelSum * AlphaBetaSum), 0.0, HALF_MAX));
		BlockMax = f16tof32(clamp(DetRcp * (BetaTexelSum * AlphaSqSum - AlphaTexelSum * AlphaBetaSum), 0.0, HALF_MAX));
	}
}

void EncodeP1(inout uint4 Block, inout float BlockMSLE, float3 Texels[16])
{
	// Compute endpoints (min/max RGB bbox)
	float3 BlockMin = Texels[0];
	float3 BlockMax = Texels[0];
	for (uint i = 1; i < 16; ++i)
	{
		BlockMin = min(BlockMin, Texels[i]);
		BlockMax = max(BlockMax, Texels[i]);
	}

	float3 BlockMinNonInset = BlockMin;
	float3 BlockMaxNonInset = BlockMax;
#if INSET_COLOR_BBOX
	InsetColorBBoxP1(Texels, BlockMin, BlockMax);
#endif

#if OPTIMIZE_ENDPOINTS
	OptimizeEndpointsP1(Texels, BlockMin, BlockMax, BlockMinNonInset, BlockMaxNonInset);
#endif

	float3 BlockDir = BlockMax - BlockMin;
	BlockDir = BlockDir / (BlockDir.x + BlockDir.y + BlockDir.z);

	float3 Endpoint0    = Quantize10(BlockMin);
	float3 Endpoint1    = Quantize10(BlockMax);
	float  EndPoint0Pos = f32tof16(dot(BlockMin, BlockDir));
	float  EndPoint1Pos = f32tof16(dot(BlockMax, BlockDir));

	// Check if endpoint swap is required
	float FixupTexelPos = f32tof16(dot(Texels[0], BlockDir));
	uint FixupIndex = ComputeIndex4(FixupTexelPos, EndPoint0Pos, EndPoint1Pos);
	if (FixupIndex > 7)
	{
		Swap(EndPoint0Pos, EndPoint1Pos);
		Swap(Endpoint0, Endpoint1);
	}

	// Compute indices
	uint Indices[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	for (uint i = 0; i < 16; ++i)
	{
		float TexelPos = f32tof16(dot(Texels[i], BlockDir));
		Indices[i] = ComputeIndex4(TexelPos, EndPoint0Pos, EndPoint1Pos);
	}

	// Compute compression error (MSLE)
	float3 Endpoint0Unq = Unquantize10(Endpoint0);
	float3 Endpoint1Unq = Unquantize10(Endpoint1);

	float MSLE = 0.0;
	for (uint i = 0; i < 16; ++i)
	{
		float Weight = floor((Indices[i] * 64.0) / 15.0 + 0.5);
		float3 texelUnc = FinishUnquantize(Endpoint0Unq, Endpoint1Unq, Weight);

		MSLE += CalcMSLE(Texels[i], texelUnc);
	}

	// Encode Block for mode 11
	BlockMSLE = MSLE;
	Block.x   = 0x03;

	// Endpoints
	Block.x |= (uint) Endpoint0.x << 5;
	Block.x |= (uint) Endpoint0.y << 15;
	Block.x |= (uint) Endpoint0.z << 25;
	Block.y |= (uint) Endpoint0.z >> 7;
	Block.y |= (uint) Endpoint1.x << 3;
	Block.y |= (uint) Endpoint1.y << 13;
	Block.y |= (uint) Endpoint1.z << 23;
	Block.z |= (uint) Endpoint1.z >> 9;

	// Indices
	Block.z |= Indices[0]  << 1;
	Block.z |= Indices[1]  << 4;
	Block.z |= Indices[2]  << 8;
	Block.z |= Indices[3]  << 12;
	Block.z |= Indices[4]  << 16;
	Block.z |= Indices[5]  << 20;
	Block.z |= Indices[6]  << 24;
	Block.z |= Indices[7]  << 28;
	Block.w |= Indices[8]  << 0;
	Block.w |= Indices[9]  << 4;
	Block.w |= Indices[10] << 8;
	Block.w |= Indices[11] << 12;
	Block.w |= Indices[12] << 16;
	Block.w |= Indices[13] << 20;
	Block.w |= Indices[14] << 24;
	Block.w |= Indices[15] << 28;
}

float DistToLineSq(float3 PointOnLine, float3 LineDirection, float3 Point)
{
	float3 w = Point - PointOnLine;
	float3 x = w - dot(w, LineDirection) * LineDirection;
	return dot(x, x);
}

// Evaluate how good is given P2 Pattern for encoding current Block
float EvaluateP2Pattern(int PatternIndex, float3 Texels[16])
{
	float3 P0BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
	float3 P0BlockMax = float3(0.0, 0.0, 0.0);
	float3 P1BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
	float3 P1BlockMax = float3(0.0, 0.0, 0.0);

	for (uint i = 0; i < 16; ++i)
	{
		uint PaletteID = Pattern(PatternIndex, i);
		if (PaletteID == 0)
		{
			P0BlockMin = min(P0BlockMin, Texels[i]);
			P0BlockMax = max(P0BlockMax, Texels[i]);
		}
		else
		{
			P1BlockMin = min(P1BlockMin, Texels[i]);
			P1BlockMax = max(P1BlockMax, Texels[i]);
		}
	}

	float3 P0BlockDir = normalize(P0BlockMax - P0BlockMin);
	float3 P1BlockDir = normalize(P1BlockMax - P1BlockMin);

	float SqDistanceFromLine = 0.0;
	for (uint i = 0; i < 16; ++i)
	{
		uint PaletteID = Pattern(PatternIndex, i);
		if (PaletteID == 0)
		{
			SqDistanceFromLine += DistToLineSq(P0BlockMin, P0BlockDir, Texels[i]);
		}
		else
		{
			SqDistanceFromLine += DistToLineSq(P1BlockMin, P1BlockDir, Texels[i]);
		}
	}

	return SqDistanceFromLine;
}

void EncodeP2Pattern(inout uint4 Block, inout float BlockMSLE, int PatternIndex, float3 Texels[16])
{
	float3 P0BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
	float3 P0BlockMax = float3(0.0, 0.0, 0.0);
	float3 P1BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
	float3 P1BlockMax = float3(0.0, 0.0, 0.0);

	for (uint i = 0; i < 16; ++i)
	{
		uint PaletteID = Pattern(PatternIndex, i);
		if (PaletteID == 0)
		{
			P0BlockMin = min(P0BlockMin, Texels[i]);
			P0BlockMax = max(P0BlockMax, Texels[i]);
		}
		else
		{
			P1BlockMin = min(P1BlockMin, Texels[i]);
			P1BlockMax = max(P1BlockMax, Texels[i]);
		}
	}

#if INSET_COLOR_BBOX
	// Disabled because it was A negligible quality increase
	//InsetColorBBoxP2(Texels, PatternIndex, 0, P0BlockMin, P0BlockMax);
	//InsetColorBBoxP2(Texels, PatternIndex, 1, P1BlockMin, P1BlockMax);
#endif

#if OPTIMIZE_ENDPOINTS
	OptimizeEndpointsP2(Texels, PatternIndex, 0, P0BlockMin, P0BlockMax);
	OptimizeEndpointsP2(Texels, PatternIndex, 1, P1BlockMin, P1BlockMax);
#endif

	float3 P0BlockDir = P0BlockMax - P0BlockMin;
	float3 P1BlockDir = P1BlockMax - P1BlockMin;
	P0BlockDir = P0BlockDir / (P0BlockDir.x + P0BlockDir.y + P0BlockDir.z);
	P1BlockDir = P1BlockDir / (P1BlockDir.x + P1BlockDir.y + P1BlockDir.z);

	float P0Endpoint0Pos = f32tof16(dot(P0BlockMin, P0BlockDir));
	float P0Endpoint1Pos = f32tof16(dot(P0BlockMax, P0BlockDir));
	float P1Endpoint0Pos = f32tof16(dot(P1BlockMin, P1BlockDir));
	float P1Endpoint1Pos = f32tof16(dot(P1BlockMax, P1BlockDir));

	uint  FixupID         = PatternFixupID(PatternIndex);
	float P0FixupTexelPos = f32tof16(dot(Texels[0], P0BlockDir));
	float P1FixupTexelPos = f32tof16(dot(Texels[FixupID], P1BlockDir));
	uint  P0FixupIndex    = ComputeIndex3(P0FixupTexelPos, P0Endpoint0Pos, P0Endpoint1Pos);
	uint  P1FixupIndex    = ComputeIndex3(P1FixupTexelPos, P1Endpoint0Pos, P1Endpoint1Pos);
	if (P0FixupIndex > 3)
	{
		Swap(P0Endpoint0Pos, P0Endpoint1Pos);
		Swap(P0BlockMin, P0BlockMax);
	}
	if (P1FixupIndex > 3)
	{
		Swap(P1Endpoint0Pos, P1Endpoint1Pos);
		Swap(P1BlockMin, P1BlockMax);
	}

	uint Indices[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	for (uint i = 0; i < 16; ++i)
	{
		float P0TexelPos = f32tof16(dot(Texels[i], P0BlockDir));
		float P1TexelPos = f32tof16(dot(Texels[i], P1BlockDir));
		uint  P0Index    = ComputeIndex3(P0TexelPos, P0Endpoint0Pos, P0Endpoint1Pos);
		uint  P1Index    = ComputeIndex3(P1TexelPos, P1Endpoint0Pos, P1Endpoint1Pos);

		uint PaletteID = Pattern(PatternIndex, i);
		Indices[i] = PaletteID == 0 ? P0Index : P1Index;
	}

	float3 Endpoint760 = floor(Quantize7(P0BlockMin));
	float3 Endpoint761 = floor(Quantize7(P0BlockMax));
	float3 Endpoint762 = floor(Quantize7(P1BlockMin));
	float3 Endpoint763 = floor(Quantize7(P1BlockMax));

	float3 Endpoint950 = floor(Quantize9(P0BlockMin));
	float3 Endpoint951 = floor(Quantize9(P0BlockMax));
	float3 Endpoint952 = floor(Quantize9(P1BlockMin));
	float3 Endpoint953 = floor(Quantize9(P1BlockMax));

	Endpoint761 = Endpoint761 - Endpoint760;
	Endpoint762 = Endpoint762 - Endpoint760;
	Endpoint763 = Endpoint763 - Endpoint760;

	Endpoint951 = Endpoint951 - Endpoint950;
	Endpoint952 = Endpoint952 - Endpoint950;
	Endpoint953 = Endpoint953 - Endpoint950;

	int MaxVal76 = 0x1F;
	Endpoint761 = clamp(Endpoint761, -MaxVal76, MaxVal76);
	Endpoint762 = clamp(Endpoint762, -MaxVal76, MaxVal76);
	Endpoint763 = clamp(Endpoint763, -MaxVal76, MaxVal76);

	int MaxVal95 = 0xF;
	Endpoint951 = clamp(Endpoint951, -MaxVal95, MaxVal95);
	Endpoint952 = clamp(Endpoint952, -MaxVal95, MaxVal95);
	Endpoint953 = clamp(Endpoint953, -MaxVal95, MaxVal95);

	float3 Endpoint760Unq = Unquantize7(Endpoint760);
	float3 Endpoint761Unq = Unquantize7(Endpoint760 + Endpoint761);
	float3 Endpoint762Unq = Unquantize7(Endpoint760 + Endpoint762);
	float3 Endpoint763Unq = Unquantize7(Endpoint760 + Endpoint763);
	float3 Endpoint950Unq = Unquantize9(Endpoint950);
	float3 Endpoint951Unq = Unquantize9(Endpoint950 + Endpoint951);
	float3 Endpoint952Unq = Unquantize9(Endpoint950 + Endpoint952);
	float3 Endpoint953Unq = Unquantize9(Endpoint950 + Endpoint953);

	float Msle76 = 0.0;
	float Msle95 = 0.0;
	for (uint i = 0; i < 16; ++i)
	{
		uint PaletteID = Pattern(PatternIndex, i);

		float3 Tmp760Unq = PaletteID == 0 ? Endpoint760Unq : Endpoint762Unq;
		float3 Tmp761Unq = PaletteID == 0 ? Endpoint761Unq : Endpoint763Unq;
		float3 Tmp950Unq = PaletteID == 0 ? Endpoint950Unq : Endpoint952Unq;
		float3 Tmp951Unq = PaletteID == 0 ? Endpoint951Unq : Endpoint953Unq;

		float  Weight     = floor((Indices[i] * 64.0) / 7.0 + 0.5);
		float3 texelUnc76 = FinishUnquantize(Tmp760Unq, Tmp761Unq, Weight);
		float3 texelUnc95 = FinishUnquantize(Tmp950Unq, Tmp951Unq, Weight);

		Msle76 += CalcMSLE(Texels[i], texelUnc76);
		Msle95 += CalcMSLE(Texels[i], texelUnc95);
	}

	SignExtend(Endpoint761, 0x1F, 0x20);
	SignExtend(Endpoint762, 0x1F, 0x20);
	SignExtend(Endpoint763, 0x1F, 0x20);

	SignExtend(Endpoint951, 0xF, 0x10);
	SignExtend(Endpoint952, 0xF, 0x10);
	SignExtend(Endpoint953, 0xF, 0x10);

	// Encode Block
	float P2MSLE = min(Msle76, Msle95);
	if (P2MSLE < BlockMSLE)
	{
		BlockMSLE = P2MSLE;
		Block     = uint4(0, 0, 0, 0);

		if (P2MSLE == Msle76)
		{
			// 7.6
			Block.x = 0x1;
			Block.x |= ((uint)Endpoint762.y & 0x20) >> 3;
			Block.x |= ((uint)Endpoint763.y & 0x10) >> 1;
			Block.x |= ((uint)Endpoint763.y & 0x20) >> 1;
			Block.x |= (uint) Endpoint760.x << 5;
			Block.x |= ((uint)Endpoint763.z & 0x01) << 12;
			Block.x |= ((uint)Endpoint763.z & 0x02) << 12;
			Block.x |= ((uint)Endpoint762.z & 0x10) << 10;
			Block.x |= (uint) Endpoint760.y << 15;
			Block.x |= ((uint)Endpoint762.z & 0x20) << 17;
			Block.x |= ((uint)Endpoint763.z & 0x04) << 21;
			Block.x |= ((uint)Endpoint762.y & 0x10) << 20;
			Block.x |= (uint) Endpoint760.z << 25;
			Block.y |= ((uint)Endpoint763.z & 0x08) >> 3;
			Block.y |= ((uint)Endpoint763.z & 0x20) >> 4;
			Block.y |= ((uint)Endpoint763.z & 0x10) >> 2;
			Block.y |= (uint) Endpoint761.x << 3;
			Block.y |= ((uint)Endpoint762.y & 0x0F) << 9;
			Block.y |= (uint) Endpoint761.y << 13;
			Block.y |= ((uint)Endpoint763.y & 0x0F) << 19;
			Block.y |= (uint) Endpoint761.z << 23;
			Block.y |= ((uint)Endpoint762.z & 0x07) << 29;
			Block.z |= ((uint)Endpoint762.z & 0x08) >> 3;
			Block.z |= (uint) Endpoint762.x << 1;
			Block.z |= (uint) Endpoint763.x << 7;
		}
		else
		{
			// 9.5
			Block.x = 0xE;
			Block.x |= (uint) Endpoint950.x << 5;
			Block.x |= ((uint)Endpoint952.z & 0x10) << 10;
			Block.x |= (uint) Endpoint950.y << 15;
			Block.x |= ((uint)Endpoint952.y & 0x10) << 20;
			Block.x |= (uint) Endpoint950.z << 25;
			Block.y |= (uint) Endpoint950.z >> 7;
			Block.y |= ((uint)Endpoint953.z & 0x10) >> 2;
			Block.y |= (uint) Endpoint951.x << 3;
			Block.y |= ((uint)Endpoint953.y & 0x10) << 4;
			Block.y |= ((uint)Endpoint952.y & 0x0F) << 9;
			Block.y |= (uint) Endpoint951.y << 13;
			Block.y |= ((uint)Endpoint953.z & 0x01) << 18;
			Block.y |= ((uint)Endpoint953.y & 0x0F) << 19;
			Block.y |= (uint) Endpoint951.z << 23;
			Block.y |= ((uint)Endpoint953.z & 0x02) << 27;
			Block.y |= (uint) Endpoint952.z << 29;
			Block.z |= ((uint)Endpoint952.z & 0x08) >> 3;
			Block.z |= (uint) Endpoint952.x << 1;
			Block.z |= ((uint)Endpoint953.z & 0x04) << 4;
			Block.z |= (uint) Endpoint953.x << 7;
			Block.z |= ((uint)Endpoint953.z & 0x08) << 9;
		}

		Block.z |= PatternIndex << 13;
		uint BlockFixupID = PatternFixupID(PatternIndex);
		if (BlockFixupID == 15)
		{
			Block.z |= Indices[0]  << 18;
			Block.z |= Indices[1]  << 20;
			Block.z |= Indices[2]  << 23;
			Block.z |= Indices[3]  << 26;
			Block.z |= Indices[4]  << 29;
			Block.w |= Indices[5]  << 0;
			Block.w |= Indices[6]  << 3;
			Block.w |= Indices[7]  << 6;
			Block.w |= Indices[8]  << 9;
			Block.w |= Indices[9]  << 12;
			Block.w |= Indices[10] << 15;
			Block.w |= Indices[11] << 18;
			Block.w |= Indices[12] << 21;
			Block.w |= Indices[13] << 24;
			Block.w |= Indices[14] << 27;
			Block.w |= Indices[15] << 30;
		}
		else if (BlockFixupID == 2)
		{
			Block.z |= Indices[0]  << 18;
			Block.z |= Indices[1]  << 20;
			Block.z |= Indices[2]  << 23;
			Block.z |= Indices[3]  << 25;
			Block.z |= Indices[4]  << 28;
			Block.z |= Indices[5]  << 31;
			Block.w |= Indices[5]  >> 1;
			Block.w |= Indices[6]  << 2;
			Block.w |= Indices[7]  << 5;
			Block.w |= Indices[8]  << 8;
			Block.w |= Indices[9]  << 11;
			Block.w |= Indices[10] << 14;
			Block.w |= Indices[11] << 17;
			Block.w |= Indices[12] << 20;
			Block.w |= Indices[13] << 23;
			Block.w |= Indices[14] << 26;
			Block.w |= Indices[15] << 29;
		}
		else
		{
			Block.z |= Indices[0]  << 18;
			Block.z |= Indices[1]  << 20;
			Block.z |= Indices[2]  << 23;
			Block.z |= Indices[3]  << 26;
			Block.z |= Indices[4]  << 29;
			Block.w |= Indices[5]  << 0;
			Block.w |= Indices[6]  << 3;
			Block.w |= Indices[7]  << 6;
			Block.w |= Indices[8]  << 9;
			Block.w |= Indices[9]  << 11;
			Block.w |= Indices[10] << 14;
			Block.w |= Indices[11] << 17;
			Block.w |= Indices[12] << 20;
			Block.w |= Indices[13] << 23;
			Block.w |= Indices[14] << 26;
			Block.w |= Indices[15] << 29;
		}
	}
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 GroupID : SV_GroupID, uint3 DispatchThreadID : SV_DispatchThreadID, uint3 GroupThreadID : SV_GroupThreadID)
{
	uint2 BlockCoord = DispatchThreadID.xy;
	if (all(BlockCoord < Constants.TextureSizeInBlocks))
	{
		// Gather Texels for current 4x4 Block
		// 0 1 2 3
		// 4 5 6 7
		// 8 9 10 11
		// 12 13 14 15
		const float2 TexCoord = BlockCoord * Constants.TextureSizeRcp * 4.0 + Constants.TextureSizeRcp;

	#ifdef ENABLE_CUBE_MAP
		float3 Block0UV = TexCoordToCubeMapDir(TexCoord, DispatchThreadID.z);
		float3 Block1UV = TexCoordToCubeMapDir(TexCoord + float2(2.0 * Constants.TextureSizeRcp.x, 0.0), DispatchThreadID.z); 
		float3 Block2UV = TexCoordToCubeMapDir(TexCoord + float2(0.0, 2.0 * Constants.TextureSizeRcp.y), DispatchThreadID.z);
		float3 Block3UV = TexCoordToCubeMapDir(TexCoord + float2(2.0 * Constants.TextureSizeRcp.x, 2.0 * Constants.TextureSizeRcp.y), DispatchThreadID.z);
	#else
		float2 Block0UV = TexCoord;
		float2 Block1UV = TexCoord + float2(2.0 * Constants.TextureSizeRcp.x, 0.0);
		float2 Block2UV = TexCoord + float2(0.0, 2.0 * Constants.TextureSizeRcp.y);
		float2 Block3UV = TexCoord + float2(2.0 * Constants.TextureSizeRcp.x, 2.0 * Constants.TextureSizeRcp.y);
	#endif

		float4 Block0X  = SourceTexture.GatherRed(PointSampler, Block0UV);
		float4 Block1X  = SourceTexture.GatherRed(PointSampler, Block1UV);
		float4 Block2X  = SourceTexture.GatherRed(PointSampler, Block2UV);
		float4 Block3X  = SourceTexture.GatherRed(PointSampler, Block3UV);
		float4 Block0Y  = SourceTexture.GatherGreen(PointSampler, Block0UV);
		float4 Block1Y  = SourceTexture.GatherGreen(PointSampler, Block1UV);
		float4 Block2Y  = SourceTexture.GatherGreen(PointSampler, Block2UV);
		float4 Block3Y  = SourceTexture.GatherGreen(PointSampler, Block3UV);
		float4 Block0Z  = SourceTexture.GatherBlue(PointSampler, Block0UV);
		float4 Block1Z  = SourceTexture.GatherBlue(PointSampler, Block1UV);
		float4 Block2Z  = SourceTexture.GatherBlue(PointSampler, Block2UV);
		float4 Block3Z  = SourceTexture.GatherBlue(PointSampler, Block3UV);

		float3 Texels[16];
		Texels[0]  = float3(Block0X.w, Block0Y.w, Block0Z.w);
		Texels[1]  = float3(Block0X.z, Block0Y.z, Block0Z.z);
		Texels[2]  = float3(Block1X.w, Block1Y.w, Block1Z.w);
		Texels[3]  = float3(Block1X.z, Block1Y.z, Block1Z.z);
		Texels[4]  = float3(Block0X.x, Block0Y.x, Block0Z.x);
		Texels[5]  = float3(Block0X.y, Block0Y.y, Block0Z.y);
		Texels[6]  = float3(Block1X.x, Block1Y.x, Block1Z.x);
		Texels[7]  = float3(Block1X.y, Block1Y.y, Block1Z.y);
		Texels[8]  = float3(Block2X.w, Block2Y.w, Block2Z.w);
		Texels[9]  = float3(Block2X.z, Block2Y.z, Block2Z.z);
		Texels[10] = float3(Block3X.w, Block3Y.w, Block3Z.w);
		Texels[11] = float3(Block3X.z, Block3Y.z, Block3Z.z);
		Texels[12] = float3(Block2X.x, Block2Y.x, Block2Z.x);
		Texels[13] = float3(Block2X.y, Block2Y.y, Block2Z.y);
		Texels[14] = float3(Block3X.x, Block3Y.x, Block3Z.x);
		Texels[15] = float3(Block3X.y, Block3Y.y, Block3Z.y);

		uint4 Block     = uint4(0, 0, 0, 0);
		float BlockMSLE = 0.0;
		EncodeP1(Block, BlockMSLE, Texels);

	#if ENCODE_P2
		// First find Pattern which is A best fit for A current Block
		float BestScore   = EvaluateP2Pattern(0, Texels);
		uint  BestPattern = 0;
		for (uint PatternIndex = 1; PatternIndex < 32; ++PatternIndex)
		{
			float Score = EvaluateP2Pattern(PatternIndex, Texels);
			if (Score < BestScore)
			{
				BestPattern = PatternIndex;
				BestScore   = Score;
			}
		}

		// Then encode it
		EncodeP2Pattern(Block, BlockMSLE, BestPattern, Texels);
	#endif

	#ifdef ENABLE_CUBE_MAP
		OutputTexture[DispatchThreadID] = Block;
	#else
		OutputTexture[BlockCoord] = Block;
	#endif
	}
}
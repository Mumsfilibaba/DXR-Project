#include "CoreDefines.hlsli"

// Modified version of: https://github.com/microsoft/DirectXTex/blob/main/DirectXTex/Shaders/BC7Encode.hlsl

#define REF_DEVICE

#define CHAR_LENGTH 8
#define NCHANNELS 4
#define BC7_UNORM 98
#define MAX_UINT 0xFFFFFFFF
#define MIN_UINT 0

// ------------------------------------------------------------------------------------------------
// BC7 Partition Tables and Interpolation Weights
// ------------------------------------------------------------------------------------------------

static const uint CandidateSectionBit[64] = // Associated to partition 0-63
{
	0xCCCC, 0x8888, 0xEEEE, 0xECC8,
	0xC880, 0xFEEC, 0xFEC8, 0xEC80,
	0xC800, 0xFFEC, 0xFE80, 0xE800,
	0xFFE8, 0xFF00, 0xFFF0, 0xF000,
	0xF710, 0x008E, 0x7100, 0x08CE,
	0x008C, 0x7310, 0x3100, 0x8CCE,
	0x088C, 0x3110, 0x6666, 0x366C,
	0x17E8, 0x0FF0, 0x718E, 0x399C,
	0xaaaa, 0xf0f0, 0x5a5a, 0x33cc,
	0x3c3c, 0x55aa, 0x9696, 0xa55a,
	0x73ce, 0x13c8, 0x324c, 0x3bdc,
	0x6996, 0xc33c, 0x9966, 0x660,
	0x272,  0x4e4,  0x4e40, 0x2720,
	0xc936, 0x936c, 0x39c6, 0x639c,
	0x9336, 0x9cc6, 0x817e, 0xe718,
	0xccf0, 0xfcc,  0x7744, 0xee22,
};

static const uint CandidateSectionBit2[64] = // Associated to partition 64-127
{
	0xaa685050, 0x6a5a5040, 0x5a5a4200, 0x5450a0a8,
	0xa5a50000, 0xa0a05050, 0x5555a0a0, 0x5a5a5050,
	0xaa550000, 0xaa555500, 0xaaaa5500, 0x90909090,
	0x94949494, 0xa4a4a4a4, 0xa9a59450, 0x2a0a4250,
	0xa5945040, 0x0a425054, 0xa5a5a500, 0x55a0a0a0,
	0xa8a85454, 0x6a6a4040, 0xa4a45000, 0x1a1a0500,
	0x0050a4a4, 0xaaa59090, 0x14696914, 0x69691400,
	0xa08585a0, 0xaa821414, 0x50a4a450, 0x6a5a0200,
	0xa9a58000, 0x5090a0a8, 0xa8a09050, 0x24242424,
	0x00aa5500, 0x24924924, 0x24499224, 0x50a50a50,
	0x500aa550, 0xaaaa4444, 0x66660000, 0xa5a0a5a0,
	0x50a050a0, 0x69286928, 0x44aaaa44, 0x66666600,
	0xaa444444, 0x54a854a8, 0x95809580, 0x96969600,
	0xa85454a8, 0x80959580, 0xaa141414, 0x96960000,
	0xaaaa1414, 0xa05050a0, 0xa0a5a5a0, 0x96000000,
	0x40804080, 0xa9a8a9a8, 0xaaaaaa44, 0x2a4a5254,
};

static const uint2 CandidateFixUpIndex1D[128] =
{
	{ 15, 0 }, { 15, 0 }, { 15, 0 }, { 15, 0 },
	{ 15, 0 }, { 15, 0 }, { 15, 0 }, { 15, 0 },
	{ 15, 0 }, { 15, 0 }, { 15, 0 }, { 15, 0 },
	{ 15, 0 }, { 15, 0 }, { 15, 0 }, { 15, 0 },
	{ 15, 0 }, {  2, 0 }, {  8, 0 }, {  2, 0 },
	{  2, 0 }, {  8, 0 }, {  8, 0 }, { 15, 0 },
	{  2, 0 }, {  8, 0 }, {  2, 0 }, {  2, 0 },
	{  8, 0 }, {  8, 0 }, {  2, 0 }, {  2, 0 },

	{ 15, 0 }, { 15, 0 }, {  6, 0 }, {  8, 0 },
	{  2, 0 }, {  8, 0 }, { 15, 0 }, { 15, 0 },
	{  2, 0 }, {  8, 0 }, {  2, 0 }, {  2, 0 },
	{  2, 0 }, { 15, 0 }, { 15, 0 }, {  6, 0 },
	{  6, 0 }, {  2, 0 }, {  6, 0 }, {  8, 0 },
	{ 15, 0 }, { 15, 0 }, {  2, 0 }, {  2, 0 },
	{ 15, 0 }, { 15, 0 }, { 15, 0 }, { 15, 0 },
	{ 15, 0 }, {  2, 0 }, {  2, 0 }, { 15, 0 },

	{  3, 15 }, {  3, 8 }, { 15, 8 }, { 15, 3 },
	{  8, 15 }, {  3, 15 }, { 15, 3 }, { 15, 8 },
	{  8, 15 }, {  8, 15 }, {  6, 15 }, {  6, 15 },
	{  6, 15 }, {  5, 15 }, {  3, 15 }, {  3, 8 },
	{  3, 15 }, {  3, 8 }, {  8, 15 }, { 15, 3 },
	{  3, 15 }, {  3, 8 }, {  6, 15 }, { 10, 8 },
	{  5, 3 }, {  8, 15 }, {  8, 6 }, {  6, 10 },
	{  8, 15 }, {  5, 15 }, { 15, 10 }, { 15, 8 },

	{  8, 15 }, { 15, 3 }, {  3, 15 }, {  5, 10 },
	{  6, 10 }, { 10, 8 }, {  8, 9 }, { 15, 10 },
	{ 15, 6 }, {  3, 15 }, { 15, 8 }, {  5, 15 },
	{ 15, 3 }, { 15, 6 }, { 15, 6 }, { 15, 8 },
	{  3, 15 }, { 15, 3 }, {  5, 15 }, {  5, 15 },
	{  5, 15 }, {  8, 15 }, {  5, 15 }, { 10, 15 },
	{  5, 15 }, { 10, 15 }, {  8, 15 }, { 13, 15 },
	{ 15, 3 }, { 12, 15 }, {  3, 15 }, {  3, 8 },
};

static const uint2 CandidateFixUpIndex1DOrdered[128] =
{
	{ 15, 0 }, { 15, 0 }, { 15, 0 }, { 15, 0 },
	{ 15, 0 }, { 15, 0 }, { 15, 0 }, { 15, 0 },
	{ 15, 0 }, { 15, 0 }, { 15, 0 }, { 15, 0 },
	{ 15, 0 }, { 15, 0 }, { 15, 0 }, { 15, 0 },
	{ 15, 0 }, {  2, 0 }, {  8, 0 }, {  2, 0 },
	{  2, 0 }, {  8, 0 }, {  8, 0 }, { 15, 0 },
	{  2, 0 }, {  8, 0 }, {  2, 0 }, {  2, 0 },
	{  8, 0 }, {  8, 0 }, {  2, 0 }, {  2, 0 },

	{ 15, 0 }, { 15, 0 }, {  6, 0 }, {  8, 0 },
	{  2, 0 }, {  8, 0 }, { 15, 0 }, { 15, 0 },
	{  2, 0 }, {  8, 0 }, {  2, 0 }, {  2, 0 },
	{  2, 0 }, { 15, 0 }, { 15, 0 }, {  6, 0 },
	{  6, 0 }, {  2, 0 }, {  6, 0 }, {  8, 0 },
	{ 15, 0 }, { 15, 0 }, {  2, 0 }, {  2, 0 },
	{ 15, 0 }, { 15, 0 }, { 15, 0 }, { 15, 0 },
	{ 15, 0 }, {  2, 0 }, {  2, 0 }, { 15, 0 },

	{  3, 15 }, {  3, 8 }, {  8, 15 }, {  3, 15 },
	{  8, 15 }, {  3, 15 }, {  3, 15 }, {  8, 15 },
	{  8, 15 }, {  8, 15 }, {  6, 15 }, {  6, 15 },
	{  6, 15 }, {  5, 15 }, {  3, 15 }, {  3, 8 },
	{  3, 15 }, {  3, 8 }, {  8, 15 }, {  3, 15 },
	{  3, 15 }, {  3, 8 }, {  6, 15 }, {  8, 10 },
	{  3, 5 }, {  8, 15 }, {  6, 8 }, {  6, 10 },
	{  8, 15 }, {  5, 15 }, { 10, 15 }, {  8, 15 },

	{  8, 15 }, {  3, 15 }, {  3, 15 }, {  5, 10 },
	{  6, 10 }, {  8, 10 }, {  8, 9 }, { 10, 15 },
	{  6, 15 }, {  3, 15 }, {  8, 15 }, {  5, 15 },
	{  3, 15 }, {  6, 15 }, {  6, 15 }, {  8, 15 },
	{  3, 15 }, {  3, 15 }, {  5, 15 }, {  5, 15 },
	{  5, 15 }, {  8, 15 }, {  5, 15 }, { 10, 15 },
	{  5, 15 }, { 10, 15 }, {  8, 15 }, { 13, 15 },
	{  3, 15 }, { 12, 15 }, {  3, 15 }, {  3, 8 },
};

static const uint InterpolationWeight[3][16] =
{
	{ 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64 },
	{ 0, 9, 18, 27, 37, 46, 55, 64, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 21, 43, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

static const uint InterpolationStep[3][64] =
{
	// 4 bit index: 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64
	{
		 0, 0, 0, 1, 1, 1, 1, 2,
		 2, 2, 2, 2, 3, 3, 3, 3,
		 4, 4, 4, 4, 5, 5, 5, 5,
		 6, 6, 6, 6, 6, 7, 7, 7,
		 7, 8, 8, 8, 8, 9, 9, 9,
		 9,10,10,10,10,10,11,11,
		11,11,12,12,12,12,13,13,
		13,13,14,14,14,14,15,15
	},
	// 3 bit index: 0, 9, 18, 27, 37, 46, 55, 64
	{
		0, 0, 0, 0, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 3,
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 5, 5, 5, 5, 5, 5,
		5, 5, 5, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 7, 7, 7, 7
	},
	// 2 bit index: 0, 21, 43, 64
	{
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3
	}
};

SHADER_CONSTANT_BLOCK_BEGIN
	// 0-16
	uint  TexWidth;
	uint  NumBlockX;
	uint  Format;
	uint  ModeId;

	// 16-28
	uint  StartBlockId;
	uint  NumTotalBlocks;
	float AlphaWeight;
SHADER_CONSTANT_BLOCK_END

// ------------------------------------------------------------------------------------------------
// Forward Declarations
// ------------------------------------------------------------------------------------------------

uint2x4 CompressEndpoints0(inout uint2x4 EndPoint, uint2 P); // Mode = 0
uint2x4 CompressEndpoints1(inout uint2x4 EndPoint, uint2 P); // Mode = 1
uint2x4 CompressEndpoints2(inout uint2x4 EndPoint);          // Mode = 2
uint2x4 CompressEndpoints3(inout uint2x4 EndPoint, uint2 P); // Mode = 3
uint2x4 CompressEndpoints7(inout uint2x4 EndPoint, uint2 P); // Mode = 7
uint2x4 CompressEndpoints6(inout uint2x4 EndPoint, uint2 P); // Mode = 6
uint2x4 CompressEndpoints4(inout uint2x4 EndPoint);          // Mode = 4
uint2x4 CompressEndpoints5(inout uint2x4 EndPoint);          // Mode = 5

void BlockPackage0(out uint4 Block, uint Partition, uint ThreadBase);                    // Mode0
void BlockPackage1(out uint4 Block, uint Partition, uint ThreadBase);                    // Mode1
void BlockPackage2(out uint4 Block, uint Partition, uint ThreadBase);                    // Mode2
void BlockPackage3(out uint4 Block, uint Partition, uint ThreadBase);                    // Mode3
void BlockPackage4(out uint4 Block, uint Rotation, uint IndexSelector, uint ThreadBase); // Mode4
void BlockPackage5(out uint4 Block, uint Rotation, uint ThreadBase);                     // Mode5
void BlockPackage6(out uint4 Block, uint ThreadBase);                                    // Mode6
void BlockPackage7(out uint4 Block, uint Partition, uint ThreadBase);                    // Mode7

// ------------------------------------------------------------------------------------------------
// Utility Functions
// ------------------------------------------------------------------------------------------------

void Swap(inout uint4 Lhs, inout uint4 Rhs)
{
	uint4 Temp = Lhs;
	Lhs = Rhs;
	Rhs = Temp;
}

void Swap(inout uint3 Lhs, inout uint3 Rhs)
{
	uint3 Temp = Lhs;
	Lhs = Rhs;
	Rhs = Temp;
}

void Swap(inout uint Lhs, inout uint Rhs)
{
	uint Temp = Lhs;
	Lhs = Rhs;
	Rhs = Temp;
}

uint ComputeError(in uint4 a, in uint4 b)
{
	return dot(a.rgb, b.rgb) + Constants.AlphaWeight * a.a * b.a;
}

void EnsureAIsLarger(inout uint4 A, inout uint4 B)
{
	if (A.x < B.x)
	{
		Swap(A.x, B.x);
	}
	if (A.y < B.y)
	{
		Swap(A.y, B.y);
	}
	if (A.z < B.z)
	{
		Swap(A.z, B.z);
	}
	if (A.w < B.w)
	{
		Swap(A.w, B.w);
	}
}

Texture2D<float4> SourceTexture : register(t0);
StructuredBuffer<uint4> InputBuffer : register(t1);

#ifndef BC7_ENCODE_ONLY
RWStructuredBuffer<uint4> OutputBuffer : register(u0);
#else
TEXTURE_FORMAT_UNKNOWN RWTexture2D<uint4> OutputTexture : register(u0);
#endif

#define THREAD_GROUP_SIZE 64
#define BLOCK_SIZE_Y 4
#define BLOCK_SIZE_X 4
#define BLOCK_SIZE (BLOCK_SIZE_Y * BLOCK_SIZE_X)

struct FSharedBlockData
{
	uint4 Pixel;
	uint  Error;
	uint  Mode;
	uint  Partition;
	uint  IndexSelector;
	uint  Rotation;
	uint4 EndPointLow;
	uint4 EndPointHigh;
	uint4 EndPointLowQuantized;
	uint4 EndPointHighQuantized;
};

groupshared FSharedBlockData SharedData[THREAD_GROUP_SIZE];

#ifndef BC7_ENCODE_ONLY
// ------------------------------------------------------------------------------------------------
// TryMode456CS - Mode 4, 5, 6 (1 subset per block, fix-up index always 0)
// ------------------------------------------------------------------------------------------------

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void TryMode456CS(uint GroupIndex : SV_GroupIndex, uint3 GroupID : SV_GroupID)
{
	const uint MAX_USED_THREAD = 16;

	uint BLOCK_IN_GROUP = THREAD_GROUP_SIZE / MAX_USED_THREAD;
	uint BlockInGroup   = GroupIndex / MAX_USED_THREAD;
	uint BlockID        = Constants.StartBlockId + GroupID.x * BLOCK_IN_GROUP + BlockInGroup;
	uint ThreadBase     = BlockInGroup * MAX_USED_THREAD;
	uint ThreadInBlock  = GroupIndex - ThreadBase;

#ifndef REF_DEVICE
	if (BlockID >= Constants.NumTotalBlocks)
	{
		return;
	}
#endif

	uint BlockY = BlockID / Constants.NumBlockX;
	uint BlockX = BlockID - BlockY * Constants.NumBlockX;
	uint BaseX  = BlockX * BLOCK_SIZE_X;
	uint BaseY  = BlockY * BLOCK_SIZE_Y;

	if (ThreadInBlock < 16)
	{
		SharedData[GroupIndex].Pixel = clamp(uint4(SourceTexture.Load(uint3(BaseX + ThreadInBlock % 4, BaseY + ThreadInBlock / 4, 0)) * 255), 0, 255);

		SharedData[GroupIndex].EndPointLow  = SharedData[GroupIndex].Pixel;
		SharedData[GroupIndex].EndPointHigh = SharedData[GroupIndex].Pixel;
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 8)
	{
		SharedData[GroupIndex].EndPointLow  = min(SharedData[GroupIndex].EndPointLow, SharedData[GroupIndex + 8].EndPointLow);
		SharedData[GroupIndex].EndPointHigh = max(SharedData[GroupIndex].EndPointHigh, SharedData[GroupIndex + 8].EndPointHigh);
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 4)
	{
		SharedData[GroupIndex].EndPointLow  = min(SharedData[GroupIndex].EndPointLow, SharedData[GroupIndex + 4].EndPointLow);
		SharedData[GroupIndex].EndPointHigh = max(SharedData[GroupIndex].EndPointHigh, SharedData[GroupIndex + 4].EndPointHigh);
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 2)
	{
		SharedData[GroupIndex].EndPointLow  = min(SharedData[GroupIndex].EndPointLow, SharedData[GroupIndex + 2].EndPointLow);
		SharedData[GroupIndex].EndPointHigh = max(SharedData[GroupIndex].EndPointHigh, SharedData[GroupIndex + 2].EndPointHigh);
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 1)
	{
		SharedData[GroupIndex].EndPointLow  = min(SharedData[GroupIndex].EndPointLow, SharedData[GroupIndex + 1].EndPointLow);
		SharedData[GroupIndex].EndPointHigh = max(SharedData[GroupIndex].EndPointHigh, SharedData[GroupIndex + 1].EndPointHigh);
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	uint2x4 EndPoint;
	EndPoint[0] = SharedData[ThreadBase].EndPointLow;
	EndPoint[1] = SharedData[ThreadBase].EndPointHigh;

	uint Error         = 0xFFFFFFFF;
	uint Mode          = 0;
	uint IndexSelector = 0;
	uint Rotation      = 0;

	uint2 IndexPrec;
	if (ThreadInBlock < 8)
	{
		if (0 == (ThreadInBlock & 1))
		{
			IndexSelector = 0;
			IndexPrec     = uint2(2, 1);
		}
		else
		{
			IndexSelector = 1;
			IndexPrec     = uint2(1, 2);
		}
	}
	else
	{
		IndexPrec = uint2(2, 2);
	}

	uint4 ReconstructedPixel;
	uint  ColorIndex;
	uint  AlphaIndex;
	int4  Span;
	int2  SpanNormSqr;
	int2  DotProduct;

	if (ThreadInBlock < 12)
	{
		if ((ThreadInBlock < 2) || (8 == ThreadInBlock))
		{
			Rotation = 0;
		}
		else if ((ThreadInBlock < 4) || (9 == ThreadInBlock))
		{
			EndPoint[0].ra = EndPoint[0].ar;
			EndPoint[1].ra = EndPoint[1].ar;
			Rotation = 1;
		}
		else if ((ThreadInBlock < 6) || (10 == ThreadInBlock))
		{
			EndPoint[0].ga = EndPoint[0].ag;
			EndPoint[1].ga = EndPoint[1].ag;
			Rotation = 2;
		}
		else if ((ThreadInBlock < 8) || (11 == ThreadInBlock))
		{
			EndPoint[0].ba = EndPoint[0].ab;
			EndPoint[1].ba = EndPoint[1].ab;
			Rotation = 3;
		}

		if (ThreadInBlock < 8)
		{
			Mode = 4;
			CompressEndpoints4(EndPoint);
		}
		else
		{
			Mode = 5;
			CompressEndpoints5(EndPoint);
		}

		uint4 Pixel = SharedData[ThreadBase + 0].Pixel;
		if (1 == Rotation)
		{
			Pixel.ra = Pixel.ar;
		}
		else if (2 == Rotation)
		{
			Pixel.ga = Pixel.ag;
		}
		else if (3 == Rotation)
		{
			Pixel.ba = Pixel.ab;
		}

		Span        = EndPoint[1] - EndPoint[0];
		SpanNormSqr = uint2(dot(Span.rgb, Span.rgb), Span.a * Span.a);

		DotProduct = int2(dot(Pixel.rgb - EndPoint[0].rgb, Pixel.rgb - EndPoint[0].rgb), dot(Pixel.rgb - EndPoint[1].rgb, Pixel.rgb - EndPoint[1].rgb));
		if (DotProduct.x > DotProduct.y)
		{
			Span.rgb = -Span.rgb;
			Swap(EndPoint[0].rgb, EndPoint[1].rgb);
		}

		DotProduct = int2(dot(Pixel.a - EndPoint[0].a, Pixel.a - EndPoint[0].a), dot(Pixel.a - EndPoint[1].a, Pixel.a - EndPoint[1].a));
		if (DotProduct.x > DotProduct.y)
		{
			Span.a = -Span.a;
			Swap(EndPoint[0].a, EndPoint[1].a);
		}

		Error = 0;
		for (uint i = 0; i < 16; i++)
		{
			Pixel = SharedData[ThreadBase + i].Pixel;
			if (1 == Rotation)
			{
				Pixel.ra = Pixel.ar;
			}
			else if (2 == Rotation)
			{
				Pixel.ga = Pixel.ag;
			}
			else if (3 == Rotation)
			{
				Pixel.ba = Pixel.ab;
			}

			DotProduct.x = dot(Span.rgb, Pixel.rgb - EndPoint[0].rgb);
			ColorIndex = (SpanNormSqr.x <= 0 || DotProduct.x <= 0) ? 0
				: ((DotProduct.x < SpanNormSqr.x) ? InterpolationStep[IndexPrec.x][uint(DotProduct.x * 63.49999 / SpanNormSqr.x)] : InterpolationStep[IndexPrec.x][63]);
			DotProduct.y = dot(Span.a, Pixel.a - EndPoint[0].a);
			AlphaIndex = (SpanNormSqr.y <= 0 || DotProduct.y <= 0) ? 0
				: ((DotProduct.y < SpanNormSqr.y) ? InterpolationStep[IndexPrec.y][uint(DotProduct.y * 63.49999 / SpanNormSqr.y)] : InterpolationStep[IndexPrec.y][63]);

			ReconstructedPixel.rgb = ((64 - InterpolationWeight[IndexPrec.x][ColorIndex]) * EndPoint[0].rgb +
				InterpolationWeight[IndexPrec.x][ColorIndex] * EndPoint[1].rgb + 32) >> 6;
			ReconstructedPixel.a = ((64 - InterpolationWeight[IndexPrec.y][AlphaIndex]) * EndPoint[0].a +
				InterpolationWeight[IndexPrec.y][AlphaIndex] * EndPoint[1].a + 32) >> 6;

			EnsureAIsLarger(ReconstructedPixel, Pixel);
			ReconstructedPixel -= Pixel;
			if (1 == Rotation)
			{
				ReconstructedPixel.ra = ReconstructedPixel.ar;
			}
			else if (2 == Rotation)
			{
				ReconstructedPixel.ga = ReconstructedPixel.ag;
			}
			else if (3 == Rotation)
			{
				ReconstructedPixel.ba = ReconstructedPixel.ab;
			}
			Error += ComputeError(ReconstructedPixel, ReconstructedPixel);
		}
	}
	else if (ThreadInBlock < 16)
	{
		uint p = ThreadInBlock - 12;

		CompressEndpoints6(EndPoint, uint2(p >> 0, p >> 1) & 1);

		uint4 Pixel = SharedData[ThreadBase + 0].Pixel;

		Span        = EndPoint[1] - EndPoint[0];
		SpanNormSqr = dot(Span, Span);
		DotProduct  = dot(Span, Pixel - EndPoint[0]);
		if (SpanNormSqr.x > 0 && DotProduct.x >= 0 && uint(DotProduct.x * 63.49999) > uint(32 * SpanNormSqr.x))
		{
			Span = -Span;
			Swap(EndPoint[0], EndPoint[1]);
		}

		Error = 0;
		for (uint i = 0; i < 16; i++)
		{
			Pixel = SharedData[ThreadBase + i].Pixel;

			DotProduct.x = dot(Span, Pixel - EndPoint[0]);
			ColorIndex = (SpanNormSqr.x <= 0 || DotProduct.x <= 0) ? 0
				: ((DotProduct.x < SpanNormSqr.x) ? InterpolationStep[0][uint(DotProduct.x * 63.49999 / SpanNormSqr.x)] : InterpolationStep[0][63]);

			ReconstructedPixel = ((64 - InterpolationWeight[0][ColorIndex]) * EndPoint[0]
				+ InterpolationWeight[0][ColorIndex] * EndPoint[1] + 32) >> 6;

			EnsureAIsLarger(ReconstructedPixel, Pixel);
			ReconstructedPixel -= Pixel;
			Error += ComputeError(ReconstructedPixel, ReconstructedPixel);
		}

		Mode     = 6;
		Rotation = p;
	}

	SharedData[GroupIndex].Error         = Error;
	SharedData[GroupIndex].Mode          = Mode;
	SharedData[GroupIndex].IndexSelector = IndexSelector;
	SharedData[GroupIndex].Rotation      = Rotation;

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 8)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 8].Error)
		{
			SharedData[GroupIndex].Error         = SharedData[GroupIndex + 8].Error;
			SharedData[GroupIndex].Mode          = SharedData[GroupIndex + 8].Mode;
			SharedData[GroupIndex].IndexSelector = SharedData[GroupIndex + 8].IndexSelector;
			SharedData[GroupIndex].Rotation      = SharedData[GroupIndex + 8].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 4)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 4].Error)
		{
			SharedData[GroupIndex].Error         = SharedData[GroupIndex + 4].Error;
			SharedData[GroupIndex].Mode          = SharedData[GroupIndex + 4].Mode;
			SharedData[GroupIndex].IndexSelector = SharedData[GroupIndex + 4].IndexSelector;
			SharedData[GroupIndex].Rotation      = SharedData[GroupIndex + 4].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 2)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 2].Error)
		{
			SharedData[GroupIndex].Error         = SharedData[GroupIndex + 2].Error;
			SharedData[GroupIndex].Mode          = SharedData[GroupIndex + 2].Mode;
			SharedData[GroupIndex].IndexSelector = SharedData[GroupIndex + 2].IndexSelector;
			SharedData[GroupIndex].Rotation      = SharedData[GroupIndex + 2].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 1)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 1].Error)
		{
			SharedData[GroupIndex].Error         = SharedData[GroupIndex + 1].Error;
			SharedData[GroupIndex].Mode          = SharedData[GroupIndex + 1].Mode;
			SharedData[GroupIndex].IndexSelector = SharedData[GroupIndex + 1].IndexSelector;
			SharedData[GroupIndex].Rotation      = SharedData[GroupIndex + 1].Rotation;
		}

		OutputBuffer[BlockID] = uint4(SharedData[GroupIndex].Error, (SharedData[GroupIndex].IndexSelector << 31) | SharedData[GroupIndex].Mode,
			0, SharedData[GroupIndex].Rotation);
	}
}

// ------------------------------------------------------------------------------------------------
// TryMode137CS - Mode 1, 3, 7 (2 subsets per block)
// ------------------------------------------------------------------------------------------------

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void TryMode137CS(uint GroupIndex : SV_GroupIndex, uint3 GroupID : SV_GroupID)
{
	const uint MAX_USED_THREAD = 64;

	uint BLOCK_IN_GROUP = THREAD_GROUP_SIZE / MAX_USED_THREAD;
	uint BlockInGroup   = GroupIndex / MAX_USED_THREAD;
	uint BlockID        = Constants.StartBlockId + GroupID.x * BLOCK_IN_GROUP + BlockInGroup;
	uint ThreadBase     = BlockInGroup * MAX_USED_THREAD;
	uint ThreadInBlock  = GroupIndex - ThreadBase;

	uint BlockY = BlockID / Constants.NumBlockX;
	uint BlockX = BlockID - BlockY * Constants.NumBlockX;
	uint BaseX  = BlockX * BLOCK_SIZE_X;
	uint BaseY  = BlockY * BLOCK_SIZE_Y;

	if (ThreadInBlock < 16)
	{
		SharedData[GroupIndex].Pixel = clamp(uint4(SourceTexture.Load(uint3(BaseX + ThreadInBlock % 4, BaseY + ThreadInBlock / 4, 0)) * 255), 0, 255);
	}
	GroupMemoryBarrierWithGroupSync();

	SharedData[GroupIndex].Error = 0xFFFFFFFF;

	uint4   ReconstructedPixel;
	uint2x4 EndPoint[2];
	uint2x4 EndPointBackup[2];
	uint    ColorIndex;
	if (ThreadInBlock < 64)
	{
		uint Partition = ThreadInBlock;

		EndPoint[0][0] = MAX_UINT;
		EndPoint[0][1] = MIN_UINT;
		EndPoint[1][0] = MAX_UINT;
		EndPoint[1][1] = MIN_UINT;
		uint Bits = CandidateSectionBit[Partition];
		for (uint i = 0; i < 16; i++)
		{
			uint4 Pixel = SharedData[ThreadBase + i].Pixel;
			if (((Bits >> i) & 0x01) == 1)
			{
				EndPoint[1][0] = min(EndPoint[1][0], Pixel);
				EndPoint[1][1] = max(EndPoint[1][1], Pixel);
			}
			else
			{
				EndPoint[0][0] = min(EndPoint[0][0], Pixel);
				EndPoint[0][1] = max(EndPoint[0][1], Pixel);
			}
		}

		EndPointBackup[0] = EndPoint[0];
		EndPointBackup[1] = EndPoint[1];

		uint MaxP;
		if (1 == Constants.ModeId)
		{
			MaxP = 2;
		}
		else
		{
			MaxP = 4;
		}

		uint FinalP[2] = { 0, 0 };
		uint Error[2]  = { MAX_UINT, MAX_UINT };
		for (uint p = 0; p < MaxP; p++)
		{
			EndPoint[0] = EndPointBackup[0];
			EndPoint[1] = EndPointBackup[1];

			for (uint i = 0; i < 2; i++)
			{
				if (Constants.ModeId == 1)
				{
					CompressEndpoints1(EndPoint[i], p);
				}
				else if (Constants.ModeId == 3)
				{
					CompressEndpoints3(EndPoint[i], uint2(p, p >> 1) & 1);
				}
				else if (Constants.ModeId == 7)
				{
					CompressEndpoints7(EndPoint[i], uint2(p, p >> 1) & 1);
				}
			}

			int4 Span[2];
			Span[0] = EndPoint[0][1] - EndPoint[0][0];
			Span[1] = EndPoint[1][1] - EndPoint[1][0];

			if (Constants.ModeId != 7)
			{
				Span[0].w = Span[1].w = 0;
			}

			int SpanNormSqr[2];
			SpanNormSqr[0] = dot(Span[0], Span[0]);
			SpanNormSqr[1] = dot(Span[1], Span[1]);

			int DotProduct = dot(Span[0], SharedData[ThreadBase + 0].Pixel - EndPoint[0][0]);
			if (SpanNormSqr[0] > 0 && DotProduct > 0 && uint(DotProduct * 63.49999) > uint(32 * SpanNormSqr[0]))
			{
				Span[0] = -Span[0];
				Swap(EndPoint[0][0], EndPoint[0][1]);
			}

			DotProduct = dot(Span[1], SharedData[ThreadBase + CandidateFixUpIndex1D[Partition].x].Pixel - EndPoint[1][0]);
			if (SpanNormSqr[1] > 0 && DotProduct > 0 && uint(DotProduct * 63.49999) > uint(32 * SpanNormSqr[1]))
			{
				Span[1] = -Span[1];
				Swap(EndPoint[1][0], EndPoint[1][1]);
			}

			uint StepSelector;
			if (Constants.ModeId != 1)
			{
				StepSelector = 2;
			}
			else
			{
				StepSelector = 1;
			}

			uint PError[2] = { 0, 0 };
			for (uint i = 0; i < 16; i++)
			{
				uint SubsetIndex = (Bits >> i) & 0x01;

				if (SubsetIndex == 1)
				{
					DotProduct = dot(Span[1], SharedData[ThreadBase + i].Pixel - EndPoint[1][0]);
					ColorIndex = (SpanNormSqr[1] <= 0 || DotProduct <= 0) ? 0
						: ((DotProduct < SpanNormSqr[1]) ? InterpolationStep[StepSelector][uint(DotProduct * 63.49999 / SpanNormSqr[1])] : InterpolationStep[StepSelector][63]);
				}
				else
				{
					DotProduct = dot(Span[0], SharedData[ThreadBase + i].Pixel - EndPoint[0][0]);
					ColorIndex = (SpanNormSqr[0] <= 0 || DotProduct <= 0) ? 0
						: ((DotProduct < SpanNormSqr[0]) ? InterpolationStep[StepSelector][uint(DotProduct * 63.49999 / SpanNormSqr[0])] : InterpolationStep[StepSelector][63]);
				}

				ReconstructedPixel = ((64 - InterpolationWeight[StepSelector][ColorIndex]) * EndPoint[SubsetIndex][0]
					+ InterpolationWeight[StepSelector][ColorIndex] * EndPoint[SubsetIndex][1] + 32) >> 6;
				if (Constants.ModeId != 7)
				{
					ReconstructedPixel.a = 255;
				}

				uint4 Pixel = SharedData[ThreadBase + i].Pixel;
				EnsureAIsLarger(ReconstructedPixel, Pixel);
				ReconstructedPixel -= Pixel;
				uint PixelError = ComputeError(ReconstructedPixel, ReconstructedPixel);
				if (SubsetIndex == 1)
				{
					PError[1] += PixelError;
				}
				else
				{
					PError[0] += PixelError;
				}
			}

			for (uint i = 0; i < 2; i++)
			{
				if (PError[i] < Error[i])
				{
					Error[i] = PError[i];
					FinalP[i] = p;
				}
			}
		}

		SharedData[GroupIndex].Error     = Error[0] + Error[1];
		SharedData[GroupIndex].Mode      = Constants.ModeId;
		SharedData[GroupIndex].Partition = Partition;

		if (Constants.ModeId == 1)
		{
			SharedData[GroupIndex].Rotation = (FinalP[1] << 1) | FinalP[0];
		}
		else
		{
			SharedData[GroupIndex].Rotation = (FinalP[1] << 2) | FinalP[0];
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if (ThreadInBlock < 32)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 32].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 32].Error;
			SharedData[GroupIndex].Mode      = SharedData[GroupIndex + 32].Mode;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 32].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 32].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 16)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 16].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 16].Error;
			SharedData[GroupIndex].Mode      = SharedData[GroupIndex + 16].Mode;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 16].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 16].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 8)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 8].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 8].Error;
			SharedData[GroupIndex].Mode      = SharedData[GroupIndex + 8].Mode;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 8].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 8].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 4)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 4].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 4].Error;
			SharedData[GroupIndex].Mode      = SharedData[GroupIndex + 4].Mode;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 4].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 4].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 2)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 2].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 2].Error;
			SharedData[GroupIndex].Mode      = SharedData[GroupIndex + 2].Mode;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 2].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 2].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 1)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 1].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 1].Error;
			SharedData[GroupIndex].Mode      = SharedData[GroupIndex + 1].Mode;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 1].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 1].Rotation;
		}

		if (InputBuffer[BlockID].x > SharedData[GroupIndex].Error)
		{
			OutputBuffer[BlockID] = uint4(SharedData[GroupIndex].Error, SharedData[GroupIndex].Mode, SharedData[GroupIndex].Partition, SharedData[GroupIndex].Rotation);
		}
		else
		{
			OutputBuffer[BlockID] = InputBuffer[BlockID];
		}
	}
}

// ------------------------------------------------------------------------------------------------
// TryMode02CS - Mode 0, 2 (3 subsets per block)
// ------------------------------------------------------------------------------------------------

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void TryMode02CS(uint GroupIndex : SV_GroupIndex, uint3 GroupID : SV_GroupID)
{
	const uint MAX_USED_THREAD = 64;

	uint BLOCK_IN_GROUP = THREAD_GROUP_SIZE / MAX_USED_THREAD;
	uint BlockInGroup   = GroupIndex / MAX_USED_THREAD;
	uint BlockID        = Constants.StartBlockId + GroupID.x * BLOCK_IN_GROUP + BlockInGroup;
	uint ThreadBase     = BlockInGroup * MAX_USED_THREAD;
	uint ThreadInBlock  = GroupIndex - ThreadBase;

	uint BlockY = BlockID / Constants.NumBlockX;
	uint BlockX = BlockID - BlockY * Constants.NumBlockX;
	uint BaseX  = BlockX * BLOCK_SIZE_X;
	uint BaseY  = BlockY * BLOCK_SIZE_Y;

	if (ThreadInBlock < 16)
	{
		SharedData[GroupIndex].Pixel = clamp(uint4(SourceTexture.Load(uint3(BaseX + ThreadInBlock % 4, BaseY + ThreadInBlock / 4, 0)) * 255), 0, 255);
	}
	GroupMemoryBarrierWithGroupSync();

	SharedData[GroupIndex].Error = 0xFFFFFFFF;

	uint NumPartitions;
	if (0 == Constants.ModeId)
	{
		NumPartitions = 16;
	}
	else
	{
		NumPartitions = 64;
	}

	uint4   ReconstructedPixel;
	uint2x4 EndPoint[3];
	uint2x4 EndPointBackup[3];
	uint    ColorIndex[16];
	if (ThreadInBlock < NumPartitions)
	{
		uint Partition = ThreadInBlock + 64;

		EndPoint[0][0] = MAX_UINT;
		EndPoint[0][1] = MIN_UINT;
		EndPoint[1][0] = MAX_UINT;
		EndPoint[1][1] = MIN_UINT;
		EndPoint[2][0] = MAX_UINT;
		EndPoint[2][1] = MIN_UINT;
		uint Bits2 = CandidateSectionBit2[Partition - 64];
		for (uint i = 0; i < 16; i++)
		{
			uint4 Pixel = SharedData[ThreadBase + i].Pixel;
			uint SubsetIndex = (Bits2 >> (i * 2)) & 0x03;
			if (SubsetIndex == 2)
			{
				EndPoint[2][0] = min(EndPoint[2][0], Pixel);
				EndPoint[2][1] = max(EndPoint[2][1], Pixel);
			}
			else if (SubsetIndex == 1)
			{
				EndPoint[1][0] = min(EndPoint[1][0], Pixel);
				EndPoint[1][1] = max(EndPoint[1][1], Pixel);
			}
			else
			{
				EndPoint[0][0] = min(EndPoint[0][0], Pixel);
				EndPoint[0][1] = max(EndPoint[0][1], Pixel);
			}
		}

		EndPointBackup[0] = EndPoint[0];
		EndPointBackup[1] = EndPoint[1];
		EndPointBackup[2] = EndPoint[2];

		uint MaxP;
		if (0 == Constants.ModeId)
		{
			MaxP = 4;
		}
		else
		{
			MaxP = 1;
		}

		uint FinalP[3] = { 0, 0, 0 };
		uint Error[3]  = { MAX_UINT, MAX_UINT, MAX_UINT };
		for (uint p = 0; p < MaxP; p++)
		{
			EndPoint[0] = EndPointBackup[0];
			EndPoint[1] = EndPointBackup[1];
			EndPoint[2] = EndPointBackup[2];

			for (uint i = 0; i < 3; i++)
			{
				if (0 == Constants.ModeId)
				{
					CompressEndpoints0(EndPoint[i], uint2(p, p >> 1) & 1);
				}
				else
				{
					CompressEndpoints2(EndPoint[i]);
				}
			}

			uint StepSelector = 1 + (2 == Constants.ModeId);

			int4 Span[3];
			Span[0] = EndPoint[0][1] - EndPoint[0][0];
			Span[1] = EndPoint[1][1] - EndPoint[1][0];
			Span[2] = EndPoint[2][1] - EndPoint[2][0];
			Span[0].w = Span[1].w = Span[2].w = 0;

			int SpanNormSqr[3];
			SpanNormSqr[0] = dot(Span[0], Span[0]);
			SpanNormSqr[1] = dot(Span[1], Span[1]);
			SpanNormSqr[2] = dot(Span[2], Span[2]);

			uint FixUpIndices[3] = { 0, CandidateFixUpIndex1D[Partition].x, CandidateFixUpIndex1D[Partition].y };
			for (uint i = 0; i < 3; i++)
			{
				int DotProduct = dot(Span[i], SharedData[ThreadBase + FixUpIndices[i]].Pixel - EndPoint[i][0]);
				if (SpanNormSqr[i] > 0 && DotProduct > 0 && uint(DotProduct * 63.49999) > uint(32 * SpanNormSqr[i]))
				{
					Span[i] = -Span[i];
					Swap(EndPoint[i][0], EndPoint[i][1]);
				}
			}

			uint PError[3] = { 0, 0, 0 };
			for (uint i = 0; i < 16; i++)
			{
				uint SubsetIndex = (Bits2 >> (i * 2)) & 0x03;
				if (SubsetIndex == 2)
				{
					int DotProduct = dot(Span[2], SharedData[ThreadBase + i].Pixel - EndPoint[2][0]);
					ColorIndex[i] = (SpanNormSqr[2] <= 0 || DotProduct <= 0) ? 0
						: ((DotProduct < SpanNormSqr[2]) ? InterpolationStep[StepSelector][uint(DotProduct * 63.49999 / SpanNormSqr[2])] : InterpolationStep[StepSelector][63]);
				}
				else if (SubsetIndex == 1)
				{
					int DotProduct = dot(Span[1], SharedData[ThreadBase + i].Pixel - EndPoint[1][0]);
					ColorIndex[i] = (SpanNormSqr[1] <= 0 || DotProduct <= 0) ? 0
						: ((DotProduct < SpanNormSqr[1]) ? InterpolationStep[StepSelector][uint(DotProduct * 63.49999 / SpanNormSqr[1])] : InterpolationStep[StepSelector][63]);
				}
				else
				{
					int DotProduct = dot(Span[0], SharedData[ThreadBase + i].Pixel - EndPoint[0][0]);
					ColorIndex[i] = (SpanNormSqr[0] <= 0 || DotProduct <= 0) ? 0
						: ((DotProduct < SpanNormSqr[0]) ? InterpolationStep[StepSelector][uint(DotProduct * 63.49999 / SpanNormSqr[0])] : InterpolationStep[StepSelector][63]);
				}

				ReconstructedPixel = ((64 - InterpolationWeight[StepSelector][ColorIndex[i]]) * EndPoint[SubsetIndex][0]
					+ InterpolationWeight[StepSelector][ColorIndex[i]] * EndPoint[SubsetIndex][1] + 32) >> 6;
				ReconstructedPixel.a = 255;

				uint4 Pixel = SharedData[ThreadBase + i].Pixel;
				EnsureAIsLarger(ReconstructedPixel, Pixel);
				ReconstructedPixel -= Pixel;

				uint PixelError = ComputeError(ReconstructedPixel, ReconstructedPixel);

				if (SubsetIndex == 2)
				{
					PError[2] += PixelError;
				}
				else if (SubsetIndex == 1)
				{
					PError[1] += PixelError;
				}
				else
				{
					PError[0] += PixelError;
				}
			}

			for (uint i = 0; i < 3; i++)
			{
				if (PError[i] < Error[i])
				{
					Error[i] = PError[i];
					FinalP[i] = p;
				}
			}
		}

		SharedData[GroupIndex].Error     = Error[0] + Error[1] + Error[2];
		SharedData[GroupIndex].Partition = Partition;
		SharedData[GroupIndex].Rotation  = (FinalP[2] << 4) | (FinalP[1] << 2) | FinalP[0];
	}
	GroupMemoryBarrierWithGroupSync();

	if (ThreadInBlock < 32)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 32].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 32].Error;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 32].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 32].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 16)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 16].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 16].Error;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 16].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 16].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 8)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 8].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 8].Error;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 8].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 8].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 4)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 4].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 4].Error;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 4].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 4].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 2)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 2].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 2].Error;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 2].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 2].Rotation;
		}
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 1)
	{
		if (SharedData[GroupIndex].Error > SharedData[GroupIndex + 1].Error)
		{
			SharedData[GroupIndex].Error     = SharedData[GroupIndex + 1].Error;
			SharedData[GroupIndex].Partition = SharedData[GroupIndex + 1].Partition;
			SharedData[GroupIndex].Rotation  = SharedData[GroupIndex + 1].Rotation;
		}

		if (InputBuffer[BlockID].x > SharedData[GroupIndex].Error)
		{
			OutputBuffer[BlockID] = uint4(SharedData[GroupIndex].Error, Constants.ModeId, SharedData[GroupIndex].Partition, SharedData[GroupIndex].Rotation);
		}
		else
		{
			OutputBuffer[BlockID] = InputBuffer[BlockID];
		}
	}
}
#endif

// ------------------------------------------------------------------------------------------------
// EncodeBlockCS - Final block packaging
// ------------------------------------------------------------------------------------------------

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void EncodeBlockCS(uint GroupIndex : SV_GroupIndex, uint3 GroupID : SV_GroupID)
{
	const uint MAX_USED_THREAD = 16;

	uint BLOCK_IN_GROUP = THREAD_GROUP_SIZE / MAX_USED_THREAD;
	uint BlockInGroup   = GroupIndex / MAX_USED_THREAD;
	uint BlockID        = Constants.StartBlockId + GroupID.x * BLOCK_IN_GROUP + BlockInGroup;
	uint ThreadBase     = BlockInGroup * MAX_USED_THREAD;
	uint ThreadInBlock  = GroupIndex - ThreadBase;

#ifndef REF_DEVICE
	if (BlockID >= Constants.NumTotalBlocks)
	{
		return;
	}
#endif

	uint BlockY = BlockID / Constants.NumBlockX;
	uint BlockX = BlockID - BlockY * Constants.NumBlockX;
	uint BaseX  = BlockX * BLOCK_SIZE_X;
	uint BaseY  = BlockY * BLOCK_SIZE_Y;

	uint Mode          = InputBuffer[BlockID].y & 0x7FFFFFFF;
	uint Partition     = InputBuffer[BlockID].z;
	uint IndexSelector = (InputBuffer[BlockID].y >> 31) & 1;
	uint Rotation      = InputBuffer[BlockID].w;

	if (ThreadInBlock < 16)
	{
		uint4 Pixel = clamp(uint4(SourceTexture.Load(uint3(BaseX + ThreadInBlock % 4, BaseY + ThreadInBlock / 4, 0)) * 255), 0, 255);

		if ((4 == Mode) || (5 == Mode))
		{
			if (1 == Rotation)
			{
				Pixel.ra = Pixel.ar;
			}
			else if (2 == Rotation)
			{
				Pixel.ga = Pixel.ag;
			}
			else if (3 == Rotation)
			{
				Pixel.ba = Pixel.ab;
			}
		}

		SharedData[GroupIndex].Pixel = Pixel;
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	uint Bits  = CandidateSectionBit[Partition];
	uint Bits2 = CandidateSectionBit2[Partition - 64];

	uint2x4 Ep;
	Ep[0] = MAX_UINT;
	Ep[1] = MIN_UINT;
	uint2x4 EpQuantized;
	[unroll]
	for (int ii = 2; ii >= 0; --ii)
	{
		if (ThreadInBlock < 16)
		{
			uint2x4 Ep;
			Ep[0] = MAX_UINT;
			Ep[1] = MIN_UINT;

			uint4 Pixel = SharedData[GroupIndex].Pixel;

			uint SubsetIndex = (Bits >> ThreadInBlock) & 0x01;
			uint SubsetIndex2 = (Bits2 >> (ThreadInBlock * 2)) & 0x03;
			if (0 == ii)
			{
				if ((0 == Mode) || (2 == Mode))
				{
					if (0 == SubsetIndex2)
					{
						Ep[0] = Ep[1] = Pixel;
					}
				}
				else if ((1 == Mode) || (3 == Mode) || (7 == Mode))
				{
					if (0 == SubsetIndex)
					{
						Ep[0] = Ep[1] = Pixel;
					}
				}
				else if ((4 == Mode) || (5 == Mode) || (6 == Mode))
				{
					Ep[0] = Ep[1] = Pixel;
				}
			}
			else if (1 == ii)
			{
				if ((0 == Mode) || (2 == Mode))
				{
					if (1 == SubsetIndex2)
					{
						Ep[0] = Ep[1] = Pixel;
					}
				}
				else if ((1 == Mode) || (3 == Mode) || (7 == Mode))
				{
					if (1 == SubsetIndex)
					{
						Ep[0] = Ep[1] = Pixel;
					}
				}
			}
			else
			{
				if ((0 == Mode) || (2 == Mode))
				{
					if (2 == SubsetIndex2)
					{
						Ep[0] = Ep[1] = Pixel;
					}
				}
			}

			SharedData[GroupIndex].EndPointLow  = Ep[0];
			SharedData[GroupIndex].EndPointHigh = Ep[1];
		}

#ifdef REF_DEVICE
		GroupMemoryBarrierWithGroupSync();
#endif

		if (ThreadInBlock < 8)
		{
			SharedData[GroupIndex].EndPointLow  = min(SharedData[GroupIndex].EndPointLow, SharedData[GroupIndex + 8].EndPointLow);
			SharedData[GroupIndex].EndPointHigh = max(SharedData[GroupIndex].EndPointHigh, SharedData[GroupIndex + 8].EndPointHigh);
		}

#ifdef REF_DEVICE
		GroupMemoryBarrierWithGroupSync();
#endif

		if (ThreadInBlock < 4)
		{
			SharedData[GroupIndex].EndPointLow  = min(SharedData[GroupIndex].EndPointLow, SharedData[GroupIndex + 4].EndPointLow);
			SharedData[GroupIndex].EndPointHigh = max(SharedData[GroupIndex].EndPointHigh, SharedData[GroupIndex + 4].EndPointHigh);
		}

#ifdef REF_DEVICE
		GroupMemoryBarrierWithGroupSync();
#endif

		if (ThreadInBlock < 2)
		{
			SharedData[GroupIndex].EndPointLow  = min(SharedData[GroupIndex].EndPointLow, SharedData[GroupIndex + 2].EndPointLow);
			SharedData[GroupIndex].EndPointHigh = max(SharedData[GroupIndex].EndPointHigh, SharedData[GroupIndex + 2].EndPointHigh);
		}

#ifdef REF_DEVICE
		GroupMemoryBarrierWithGroupSync();
#endif

		if (ThreadInBlock < 1)
		{
			SharedData[GroupIndex].EndPointLow  = min(SharedData[GroupIndex].EndPointLow, SharedData[GroupIndex + 1].EndPointLow);
			SharedData[GroupIndex].EndPointHigh = max(SharedData[GroupIndex].EndPointHigh, SharedData[GroupIndex + 1].EndPointHigh);
		}

#ifdef REF_DEVICE
		GroupMemoryBarrierWithGroupSync();
#endif

		if (ii == (int)ThreadInBlock)
		{
			Ep[0] = SharedData[ThreadBase].EndPointLow;
			Ep[1] = SharedData[ThreadBase].EndPointHigh;
		}
	}

	if (ThreadInBlock < 3)
	{
		uint2 P;
		if (1 == Mode)
		{
			P = (Rotation >> ThreadInBlock) & 1;
		}
		else
		{
			P = uint2(Rotation >> (ThreadInBlock * 2 + 0), Rotation >> (ThreadInBlock * 2 + 1)) & 1;
		}

		if (0 == Mode)
		{
			EpQuantized = CompressEndpoints0(Ep, P);
		}
		else if (1 == Mode)
		{
			EpQuantized = CompressEndpoints1(Ep, P);
		}
		else if (2 == Mode)
		{
			EpQuantized = CompressEndpoints2(Ep);
		}
		else if (3 == Mode)
		{
			EpQuantized = CompressEndpoints3(Ep, P);
		}
		else if (4 == Mode)
		{
			EpQuantized = CompressEndpoints4(Ep);
		}
		else if (5 == Mode)
		{
			EpQuantized = CompressEndpoints5(Ep);
		}
		else if (6 == Mode)
		{
			EpQuantized = CompressEndpoints6(Ep, P);
		}
		else
		{
			EpQuantized = CompressEndpoints7(Ep, P);
		}

		int4 Span = Ep[1] - Ep[0];
		if (Mode < 4)
		{
			Span.w = 0;
		}

		if ((4 == Mode) || (5 == Mode))
		{
			if (0 == ThreadInBlock)
			{
				int2 SpanNormSqr = uint2(dot(Span.rgb, Span.rgb), Span.a * Span.a);
				int2 DotProduct  = int2(dot(Span.rgb, SharedData[ThreadBase + 0].Pixel.rgb - Ep[0].rgb), Span.a * (SharedData[ThreadBase + 0].Pixel.a - Ep[0].a));
				if (SpanNormSqr.x > 0 && DotProduct.x > 0 && uint(DotProduct.x * 63.49999) > uint(32 * SpanNormSqr.x))
				{
					Swap(Ep[0].rgb, Ep[1].rgb);
					Swap(EpQuantized[0].rgb, EpQuantized[1].rgb);
				}

				if (SpanNormSqr.y > 0 && DotProduct.y > 0 && uint(DotProduct.y * 63.49999) > uint(32 * SpanNormSqr.y))
				{
					Swap(Ep[0].a, Ep[1].a);
					Swap(EpQuantized[0].a, EpQuantized[1].a);
				}
			}
		}
		else
		{
			int p;
			if (0 == ThreadInBlock)
			{
				p = 0;
			}
			else if (1 == ThreadInBlock)
			{
				p = CandidateFixUpIndex1D[Partition].x;
			}
			else
			{
				p = CandidateFixUpIndex1D[Partition].y;
			}

			int SpanNormSqr = dot(Span, Span);
			int DotProduct  = dot(Span, SharedData[ThreadBase + p].Pixel - Ep[0]);
			if (SpanNormSqr > 0 && DotProduct > 0 && uint(DotProduct * 63.49999) > uint(32 * SpanNormSqr))
			{
				Swap(Ep[0], Ep[1]);
				Swap(EpQuantized[0], EpQuantized[1]);
			}
		}

		SharedData[GroupIndex].EndPointLow            = Ep[0];
		SharedData[GroupIndex].EndPointHigh           = Ep[1];
		SharedData[GroupIndex].EndPointLowQuantized   = EpQuantized[0];
		SharedData[GroupIndex].EndPointHighQuantized  = EpQuantized[1];
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (ThreadInBlock < 16)
	{
		uint ColorIndex = 0;
		uint AlphaIndex = 0;

		uint2x4 Ep;

		uint2 IndexPrec;
		if ((0 == Mode) || (1 == Mode))
		{
			IndexPrec = 1;
		}
		else if (6 == Mode)
		{
			IndexPrec = 0;
		}
		else if (4 == Mode)
		{
			if (0 == IndexSelector)
			{
				IndexPrec = uint2(2, 1);
			}
			else
			{
				IndexPrec = uint2(1, 2);
			}
		}
		else
		{
			IndexPrec = 2;
		}

		int SubsetIndex;
		if ((0 == Mode) || (2 == Mode))
		{
			SubsetIndex = (Bits2 >> (ThreadInBlock * 2)) & 0x03;
		}
		else if ((1 == Mode) || (3 == Mode) || (7 == Mode))
		{
			SubsetIndex = (Bits >> ThreadInBlock) & 0x01;
		}
		else
		{
			SubsetIndex = 0;
		}

		Ep[0] = SharedData[ThreadBase + SubsetIndex].EndPointLow;
		Ep[1] = SharedData[ThreadBase + SubsetIndex].EndPointHigh;

		int4 Span = Ep[1] - Ep[0];
		if (Mode < 4)
		{
			Span.w = 0;
		}

		if ((4 == Mode) || (5 == Mode))
		{
			int2 SpanNormSqr;
			SpanNormSqr.x = dot(Span.rgb, Span.rgb);
			SpanNormSqr.y = Span.a * Span.a;

			int DotProduct = dot(Span.rgb, SharedData[ThreadBase + ThreadInBlock].Pixel.rgb - Ep[0].rgb);
			ColorIndex = (SpanNormSqr.x <= 0 || DotProduct <= 0) ? 0
				: ((DotProduct < SpanNormSqr.x) ? InterpolationStep[IndexPrec.x][uint(DotProduct * 63.49999 / SpanNormSqr.x)] : InterpolationStep[IndexPrec.x][63]);
			DotProduct = dot(Span.a, SharedData[ThreadBase + ThreadInBlock].Pixel.a - Ep[0].a);
			AlphaIndex = (SpanNormSqr.y <= 0 || DotProduct <= 0) ? 0
				: ((DotProduct < SpanNormSqr.y) ? InterpolationStep[IndexPrec.y][uint(DotProduct * 63.49999 / SpanNormSqr.y)] : InterpolationStep[IndexPrec.y][63]);

			if (IndexSelector)
			{
				Swap(ColorIndex, AlphaIndex);
			}
		}
		else
		{
			int SpanNormSqr = dot(Span, Span);
			int DotProduct  = dot(Span, SharedData[ThreadBase + ThreadInBlock].Pixel - Ep[0]);
			ColorIndex = (SpanNormSqr <= 0 || DotProduct <= 0) ? 0
				: ((DotProduct < SpanNormSqr) ? InterpolationStep[IndexPrec.x][uint(DotProduct * 63.49999 / SpanNormSqr)] : InterpolationStep[IndexPrec.x][63]);
		}

		SharedData[GroupIndex].Error = ColorIndex;
		SharedData[GroupIndex].Mode  = AlphaIndex;
	}

#ifdef REF_DEVICE
	GroupMemoryBarrierWithGroupSync();
#endif

	if (0 == ThreadInBlock)
	{
		uint4 Block;
		if (0 == Mode)
		{
			BlockPackage0(Block, Partition, ThreadBase);
		}
		else if (1 == Mode)
		{
			BlockPackage1(Block, Partition, ThreadBase);
		}
		else if (2 == Mode)
		{
			BlockPackage2(Block, Partition, ThreadBase);
		}
		else if (3 == Mode)
		{
			BlockPackage3(Block, Partition, ThreadBase);
		}
		else if (4 == Mode)
		{
			BlockPackage4(Block, Rotation, IndexSelector, ThreadBase);
		}
		else if (5 == Mode)
		{
			BlockPackage5(Block, Rotation, ThreadBase);
		}
		else if (6 == Mode)
		{
			BlockPackage6(Block, ThreadBase);
		}
		else
		{
			BlockPackage7(Block, Partition, ThreadBase);
		}

#ifdef BC7_ENCODE_ONLY
		OutputTexture[uint2(BlockX, BlockY)] = Block;
#else
		OutputBuffer[BlockID] = Block;
#endif
	}
}

// ------------------------------------------------------------------------------------------------
// Quantization and Endpoint Compression
// ------------------------------------------------------------------------------------------------

uint4 Quantize(uint4 Color, uint Precision)
{
	return (((Color << 8) + Color) * ((1U << Precision) - 1) + 32768) >> 16;
}

uint4 Unquantize(uint4 Color, uint Precision)
{
	Color = Color << (8 - Precision);
	return Color | (Color >> Precision);
}

uint2x4 CompressEndpoints0(inout uint2x4 EndPoint, uint2 P)
{
	uint2x4 Quantized;
	[unroll] for (uint j = 0; j < 2; j++)
	{
		Quantized[j].rgb = Quantize(EndPoint[j].rgbb, 5).rgb & 0xFFFFFFFE;
		Quantized[j].rgb |= P[j];
		Quantized[j].a = 0xFF;

		EndPoint[j].rgb = Unquantize(Quantized[j].rgbb, 5).rgb;
		EndPoint[j].a = 0xFF;

		Quantized[j] <<= 3;
	}
	
	return Quantized;
}

uint2x4 CompressEndpoints1(inout uint2x4 EndPoint, uint2 P)
{
	uint2x4 Quantized;
	[unroll] for (uint j = 0; j < 2; j++)
	{
		Quantized[j].rgb = Quantize(EndPoint[j].rgbb, 7).rgb & 0xFFFFFFFE;
		Quantized[j].rgb |= P[j];
		Quantized[j].a = 0xFF;

		EndPoint[j].rgb = Unquantize(Quantized[j].rgbb, 7).rgb;
		EndPoint[j].a = 0xFF;

		Quantized[j] <<= 1;
	}

	return Quantized;
}

uint2x4 CompressEndpoints2(inout uint2x4 EndPoint)
{
	uint2x4 Quantized;
	[unroll] for (uint j = 0; j < 2; j++)
	{
		Quantized[j].rgb = Quantize(EndPoint[j].rgbb, 5).rgb;
		Quantized[j].a = 0xFF;

		EndPoint[j].rgb = Unquantize(Quantized[j].rgbb, 5).rgb;
		EndPoint[j].a = 0xFF;

		Quantized[j] <<= 3;
	}

	return Quantized;
}

uint2x4 CompressEndpoints3(inout uint2x4 EndPoint, uint2 P)
{
	uint2x4 Quantized;
	for (uint j = 0; j < 2; j++)
	{
		Quantized[j].rgb = EndPoint[j].rgb & 0xFFFFFFFE;
		Quantized[j].rgb |= P[j];
		Quantized[j].a = 0xFF;

		EndPoint[j].rgb = Quantized[j].rgb;
		EndPoint[j].a = 0xFF;
	}

	return Quantized;
}

uint2x4 CompressEndpoints4(inout uint2x4 EndPoint)
{
	uint2x4 Quantized;
	[unroll] for (uint j = 0; j < 2; j++)
	{
		Quantized[j].rgb = Quantize(EndPoint[j].rgbb, 5).rgb;
		Quantized[j].a = Quantize(EndPoint[j].a, 6).r;

		EndPoint[j].rgb = Unquantize(Quantized[j].rgbb, 5).rgb;
		EndPoint[j].a = Unquantize(Quantized[j].a, 6).r;

		Quantized[j].rgb <<= 3;
		Quantized[j].a <<= 2;
	}

	return Quantized;
}

uint2x4 CompressEndpoints5(inout uint2x4 EndPoint)
{
	uint2x4 Quantized;
	[unroll] for (uint j = 0; j < 2; j++)
	{
		Quantized[j].rgb = Quantize(EndPoint[j].rgbb, 7).rgb;
		Quantized[j].a = EndPoint[j].a;

		EndPoint[j].rgb = Unquantize(Quantized[j].rgbb, 7).rgb;

		Quantized[j].rgb <<= 1;
	}

	return Quantized;
}

uint2x4 CompressEndpoints6(inout uint2x4 EndPoint, uint2 P)
{
	uint2x4 Quantized;
	for (uint j = 0; j < 2; j++)
	{
		Quantized[j] = EndPoint[j] & 0xFFFFFFFE;
		Quantized[j] |= P[j];

		EndPoint[j] = Quantized[j];
	}

	return Quantized;
}

uint2x4 CompressEndpoints7(inout uint2x4 EndPoint, uint2 P)
{
	uint2x4 Quantized;
	[unroll] for (uint j = 0; j < 2; j++)
	{
		Quantized[j] = Quantize(EndPoint[j], 6) & 0xFFFFFFFE;
		Quantized[j] |= P[j];

		EndPoint[j] = Unquantize(Quantized[j], 6);
	}

	return uint2x4(Quantized[0] << 2, Quantized[1] << 2);
}

// ------------------------------------------------------------------------------------------------
// Block Packaging (Mode 0-7)
// ------------------------------------------------------------------------------------------------

#define GetEndPointL(Subset) SharedData[ThreadBase + Subset].EndPointLowQuantized
#define GetEndPointH(Subset) SharedData[ThreadBase + Subset].EndPointHighQuantized
#define GetColorIndex(Index) SharedData[ThreadBase + Index].Error
#define GetAlphaIndex(Index) SharedData[ThreadBase + Index].Mode

void BlockPackage0(out uint4 Block, uint Partition, uint ThreadBase)
{
	Block.x = 0x01 | ((Partition - 64) << 1)
		| ((GetEndPointL(0).r & 0xF0) << 1) | ((GetEndPointH(0).r & 0xF0) << 5)
		| ((GetEndPointL(1).r & 0xF0) << 9) | ((GetEndPointH(1).r & 0xF0) << 13)
		| ((GetEndPointL(2).r & 0xF0) << 17) | ((GetEndPointH(2).r & 0xF0) << 21)
		| ((GetEndPointL(0).g & 0xF0) << 25);
	Block.y = ((GetEndPointL(0).g & 0xF0) >> 7) | ((GetEndPointH(0).g & 0xF0) >> 3)
		| ((GetEndPointL(1).g & 0xF0) << 1) | ((GetEndPointH(1).g & 0xF0) << 5)
		| ((GetEndPointL(2).g & 0xF0) << 9) | ((GetEndPointH(2).g & 0xF0) << 13)
		| ((GetEndPointL(0).b & 0xF0) << 17) | ((GetEndPointH(0).b & 0xF0) << 21)
		| ((GetEndPointL(1).b & 0xF0) << 25);
	Block.z = ((GetEndPointL(1).b & 0xF0) >> 7) | ((GetEndPointH(1).b & 0xF0) >> 3)
		| ((GetEndPointL(2).b & 0xF0) << 1) | ((GetEndPointH(2).b & 0xF0) << 5)
		| ((GetEndPointL(0).r & 0x08) << 10) | ((GetEndPointH(0).r & 0x08) << 11)
		| ((GetEndPointL(1).r & 0x08) << 12) | ((GetEndPointH(1).r & 0x08) << 13)
		| ((GetEndPointL(2).r & 0x08) << 14) | ((GetEndPointH(2).r & 0x08) << 15)
		| (GetColorIndex(0) << 19);
	Block.w = 0;
	
	uint i = 1;
	for (; i <= min(CandidateFixUpIndex1DOrdered[Partition][0], 4); i++)
	{
		Block.z |= GetColorIndex(i) << (i * 3 + 18);
	}

	if (CandidateFixUpIndex1DOrdered[Partition][0] < 4)
	{
		Block.z |= GetColorIndex(4) << 29;
		i += 1;
	}
	else
	{
		Block.w |= (GetColorIndex(4) & 0x04) >> 2;
		for (; i <= CandidateFixUpIndex1DOrdered[Partition][0]; i++)
			Block.w |= GetColorIndex(i) << (i * 3 - 14);
	}
	
	for (; i <= CandidateFixUpIndex1DOrdered[Partition][1]; i++)
	{
		Block.w |= GetColorIndex(i) << (i * 3 - 15);
	}

	for (; i < 16; i++)
	{
		Block.w |= GetColorIndex(i) << (i * 3 - 16);
	}
}
void BlockPackage1(out uint4 Block, uint Partition, uint ThreadBase)
{
	Block.x = 0x02 | (Partition << 2)
		| ((GetEndPointL(0).r & 0xFC) << 6) | ((GetEndPointH(0).r & 0xFC) << 12)
		| ((GetEndPointL(1).r & 0xFC) << 18) | ((GetEndPointH(1).r & 0xFC) << 24);
	Block.y = ((GetEndPointL(0).g & 0xFC) >> 2) | ((GetEndPointH(0).g & 0xFC) << 4)
		| ((GetEndPointL(1).g & 0xFC) << 10) | ((GetEndPointH(1).g & 0xFC) << 16)
		| ((GetEndPointL(0).b & 0xFC) << 22) | ((GetEndPointH(0).b & 0xFC) << 28);
	Block.z = ((GetEndPointH(0).b & 0xFC) >> 4) | ((GetEndPointL(1).b & 0xFC) << 2)
		| ((GetEndPointH(1).b & 0xFC) << 8)
		| ((GetEndPointL(0).r & 0x02) << 15) | ((GetEndPointL(1).r & 0x02) << 16)
		| (GetColorIndex(0) << 18);

	if (CandidateFixUpIndex1DOrdered[Partition][0] == 15)
	{
		Block.w = (GetColorIndex(15) << 30) | (GetColorIndex(14) << 27) | (GetColorIndex(13) << 24) | (GetColorIndex(12) << 21) | (GetColorIndex(11) << 18) | (GetColorIndex(10) << 15)
			| (GetColorIndex(9) << 12) | (GetColorIndex(8) << 9) | (GetColorIndex(7) << 6) | (GetColorIndex(6) << 3) | GetColorIndex(5);
		Block.z |= (GetColorIndex(4) << 29) | (GetColorIndex(3) << 26) | (GetColorIndex(2) << 23) | (GetColorIndex(1) << 20) | (GetColorIndex(0) << 18);
	}
	else if (CandidateFixUpIndex1DOrdered[Partition][0] == 2)
	{
		Block.w = (GetColorIndex(15) << 29) | (GetColorIndex(14) << 26) | (GetColorIndex(13) << 23) | (GetColorIndex(12) << 20) | (GetColorIndex(11) << 17) | (GetColorIndex(10) << 14)
			| (GetColorIndex(9) << 11) | (GetColorIndex(8) << 8) | (GetColorIndex(7) << 5) | (GetColorIndex(6) << 2) | (GetColorIndex(5) >> 1);
		Block.z |= (GetColorIndex(5) << 31) | (GetColorIndex(4) << 28) | (GetColorIndex(3) << 25) | (GetColorIndex(2) << 23) | (GetColorIndex(1) << 20) | (GetColorIndex(0) << 18);
	}
	else if (CandidateFixUpIndex1DOrdered[Partition][0] == 8)
	{
		Block.w = (GetColorIndex(15) << 29) | (GetColorIndex(14) << 26) | (GetColorIndex(13) << 23) | (GetColorIndex(12) << 20) | (GetColorIndex(11) << 17) | (GetColorIndex(10) << 14)
			| (GetColorIndex(9) << 11) | (GetColorIndex(8) << 9) | (GetColorIndex(7) << 6) | (GetColorIndex(6) << 3) | GetColorIndex(5);
		Block.z |= (GetColorIndex(4) << 29) | (GetColorIndex(3) << 26) | (GetColorIndex(2) << 23) | (GetColorIndex(1) << 20) | (GetColorIndex(0) << 18);
	}
	else // CandidateFixUpIndex1DOrdered[Partition] == 6
	{
		Block.w = (GetColorIndex(15) << 29) | (GetColorIndex(14) << 26) | (GetColorIndex(13) << 23) | (GetColorIndex(12) << 20) | (GetColorIndex(11) << 17) | (GetColorIndex(10) << 14)
			| (GetColorIndex(9) << 11) | (GetColorIndex(8) << 8) | (GetColorIndex(7) << 5) | (GetColorIndex(6) << 3) | GetColorIndex(5);
		Block.z |= (GetColorIndex(4) << 29) | (GetColorIndex(3) << 26) | (GetColorIndex(2) << 23) | (GetColorIndex(1) << 20) | (GetColorIndex(0) << 18);
	}
}
void BlockPackage2(out uint4 Block, uint Partition, uint ThreadBase)
{
	Block.x = 0x04 | ((Partition - 64) << 3)
		| ((GetEndPointL(0).r & 0xF8) << 6) | ((GetEndPointH(0).r & 0xF8) << 11)
		| ((GetEndPointL(1).r & 0xF8) << 16) | ((GetEndPointH(1).r & 0xF8) << 21)
		| ((GetEndPointL(2).r & 0xF8) << 26);
	Block.y = ((GetEndPointL(2).r & 0xF8) >> 6) | ((GetEndPointH(2).r & 0xF8) >> 1)
		| ((GetEndPointL(0).g & 0xF8) << 4) | ((GetEndPointH(0).g & 0xF8) << 9)
		| ((GetEndPointL(1).g & 0xF8) << 14) | ((GetEndPointH(1).g & 0xF8) << 19)
		| ((GetEndPointL(2).g & 0xF8) << 24);
	Block.z = ((GetEndPointH(2).g & 0xF8) >> 3) | ((GetEndPointL(0).b & 0xF8) << 2)
		| ((GetEndPointH(0).b & 0xF8) << 7) | ((GetEndPointL(1).b & 0xF8) << 12)
		| ((GetEndPointH(1).b & 0xF8) << 17) | ((GetEndPointL(2).b & 0xF8) << 22)
		| ((GetEndPointH(2).b & 0xF8) << 27);
	Block.w = ((GetEndPointH(2).b & 0xF8) >> 5)
		| (GetColorIndex(0) << 3);
	
	uint i = 1;
	for (; i <= CandidateFixUpIndex1DOrdered[Partition][0]; i++)
	{
		Block.w |= GetColorIndex(i) << (i * 2 + 2);
	}
	
	for (; i <= CandidateFixUpIndex1DOrdered[Partition][1]; i++)
	{
		Block.w |= GetColorIndex(i) << (i * 2 + 1);
	}

	for (; i < 16; i++)
	{
		Block.w |= GetColorIndex(i) << (i * 2);
	}
}
void BlockPackage3(out uint4 Block, uint Partition, uint ThreadBase)
{
	Block.x = 0x08 | (Partition << 4)
		| ((GetEndPointL(0).r & 0xFE) << 9) | ((GetEndPointH(0).r & 0xFE) << 16)
		| ((GetEndPointL(1).r & 0xFE) << 23) | ((GetEndPointH(1).r & 0xFE) << 30);
	Block.y = ((GetEndPointH(1).r & 0xFE) >> 2) | ((GetEndPointL(0).g & 0xFE) << 5)
		| ((GetEndPointH(0).g & 0xFE) << 12) | ((GetEndPointL(1).g & 0xFE) << 19)
		| ((GetEndPointH(1).g & 0xFE) << 26);
	Block.z = ((GetEndPointH(1).g & 0xFE) >> 6) | ((GetEndPointL(0).b & 0xFE) << 1)
		| ((GetEndPointH(0).b & 0xFE) << 8) | ((GetEndPointL(1).b & 0xFE) << 15)
		| ((GetEndPointH(1).b & 0xFE) << 22)
		| ((GetEndPointL(0).r & 0x01) << 30) | ((GetEndPointH(0).r & 0x01) << 31);
	Block.w = ((GetEndPointL(1).r & 0x01) << 0) | ((GetEndPointH(1).r & 0x01) << 1)
		| (GetColorIndex(0) << 2);
	
	uint i = 1;
	for (; i <= CandidateFixUpIndex1DOrdered[Partition][0]; i++)
	{
		Block.w |= GetColorIndex(i) << (i * 2 + 1);
	}

	for (; i < 16; i++)
	{
		Block.w |= GetColorIndex(i) << (i * 2);
	}
}

void BlockPackage4(out uint4 Block, uint Rotation, uint IndexSelector, uint ThreadBase)
{
	Block.x = 0x10 | ((Rotation & 3) << 5) | ((IndexSelector & 1) << 7)
		| ((GetEndPointL(0).r & 0xF8) << 5) | ((GetEndPointH(0).r & 0xF8) << 10)
		| ((GetEndPointL(0).g & 0xF8) << 15) | ((GetEndPointH(0).g & 0xF8) << 20)
		| ((GetEndPointL(0).b & 0xF8) << 25);

	Block.y = ((GetEndPointL(0).b & 0xF8) >> 7) | ((GetEndPointH(0).b & 0xF8) >> 2)
		| ((GetEndPointL(0).a & 0xFC) << 4) | ((GetEndPointH(0).a & 0xFC) << 10)
		| ((GetColorIndex(0) & 1) << 18) | (GetColorIndex(1) << 19) | (GetColorIndex(2) << 21) | (GetColorIndex(3) << 23)
		| (GetColorIndex(4) << 25) | (GetColorIndex(5) << 27) | (GetColorIndex(6) << 29) | (GetColorIndex(7) << 31);

	Block.z = (GetColorIndex(7) >> 1) | (GetColorIndex(8) << 1) | (GetColorIndex(9) << 3) | (GetColorIndex(10) << 5)
		| (GetColorIndex(11) << 7) | (GetColorIndex(12) << 9) | (GetColorIndex(13) << 11) | (GetColorIndex(14) << 13)
		| (GetColorIndex(15) << 15) | ((GetAlphaIndex(0) & 3) << 17) | (GetAlphaIndex(1) << 19) | (GetAlphaIndex(2) << 22)
		| (GetAlphaIndex(3) << 25) | (GetAlphaIndex(4) << 28) | (GetAlphaIndex(5) << 31);

	Block.w = (GetAlphaIndex(5) >> 1) | (GetAlphaIndex(6) << 2) | (GetAlphaIndex(7) << 5) | (GetAlphaIndex(8) << 8)
		| (GetAlphaIndex(9) << 11) | (GetAlphaIndex(10) << 14) | (GetAlphaIndex(11) << 17) | (GetAlphaIndex(12) << 20)
		| (GetAlphaIndex(13) << 23) | (GetAlphaIndex(14) << 26) | (GetAlphaIndex(15) << 29);
}

void BlockPackage5(out uint4 Block, uint Rotation, uint ThreadBase)
{
	Block.x = 0x20 | (Rotation << 6)
		| ((GetEndPointL(0).r & 0xFE) << 7) | ((GetEndPointH(0).r & 0xFE) << 14)
		| ((GetEndPointL(0).g & 0xFE) << 21) | ((GetEndPointH(0).g & 0xFE) << 28);
	Block.y = ((GetEndPointH(0).g & 0xFE) >> 4) | ((GetEndPointL(0).b & 0xFE) << 3)
		| ((GetEndPointH(0).b & 0xFE) << 10) | (GetEndPointL(0).a << 18) | (GetEndPointH(0).a << 26);
	Block.z = (GetEndPointH(0).a >> 6)
		| (GetColorIndex(0) << 2) | (GetColorIndex(1) << 3) | (GetColorIndex(2) << 5) | (GetColorIndex(3) << 7)
		| (GetColorIndex(4) << 9) | (GetColorIndex(5) << 11) | (GetColorIndex(6) << 13) | (GetColorIndex(7) << 15)
		| (GetColorIndex(8) << 17) | (GetColorIndex(9) << 19) | (GetColorIndex(10) << 21) | (GetColorIndex(11) << 23)
		| (GetColorIndex(12) << 25) | (GetColorIndex(13) << 27) | (GetColorIndex(14) << 29) | (GetColorIndex(15) << 31);
	Block.w = (GetColorIndex(15) >> 1) | (GetAlphaIndex(0) << 1) | (GetAlphaIndex(1) << 2) | (GetAlphaIndex(2) << 4)
		| (GetAlphaIndex(3) << 6) | (GetAlphaIndex(4) << 8) | (GetAlphaIndex(5) << 10) | (GetAlphaIndex(6) << 12)
		| (GetAlphaIndex(7) << 14) | (GetAlphaIndex(8) << 16) | (GetAlphaIndex(9) << 18) | (GetAlphaIndex(10) << 20)
		| (GetAlphaIndex(11) << 22) | (GetAlphaIndex(12) << 24) | (GetAlphaIndex(13) << 26) | (GetAlphaIndex(14) << 28)
		| (GetAlphaIndex(15) << 30);
}

void BlockPackage6(out uint4 Block, uint ThreadBase)
{
	Block.x = 0x40
		| ((GetEndPointL(0).r & 0xFE) << 6) | ((GetEndPointH(0).r & 0xFE) << 13)
		| ((GetEndPointL(0).g & 0xFE) << 20) | ((GetEndPointH(0).g & 0xFE) << 27);
	Block.y = ((GetEndPointH(0).g & 0xFE) >> 5) | ((GetEndPointL(0).b & 0xFE) << 2)
		| ((GetEndPointH(0).b & 0xFE) << 9) | ((GetEndPointL(0).a & 0xFE) << 16)
		| ((GetEndPointH(0).a & 0xFE) << 23)
		| ((GetEndPointL(0).r & 0x01) << 31);
	Block.z = (GetEndPointH(0).r & 0x01)
		| (GetColorIndex(0) << 1) | (GetColorIndex(1) << 4) | (GetColorIndex(2) << 8) | (GetColorIndex(3) << 12)
		| (GetColorIndex(4) << 16) | (GetColorIndex(5) << 20) | (GetColorIndex(6) << 24) | (GetColorIndex(7) << 28);
	Block.w = (GetColorIndex(8) << 0) | (GetColorIndex(9) << 4) | (GetColorIndex(10) << 8) | (GetColorIndex(11) << 12)
		| (GetColorIndex(12) << 16) | (GetColorIndex(13) << 20) | (GetColorIndex(14) << 24) | (GetColorIndex(15) << 28);
}

void BlockPackage7(out uint4 Block, uint Partition, uint ThreadBase)
{
	Block.x = 0x80 | (Partition << 8)
		| ((GetEndPointL(0).r & 0xF8) << 11) | ((GetEndPointH(0).r & 0xF8) << 16)
		| ((GetEndPointL(1).r & 0xF8) << 21) | ((GetEndPointH(1).r & 0xF8) << 26);
	Block.y = ((GetEndPointH(1).r & 0xF8) >> 6) | ((GetEndPointL(0).g & 0xF8) >> 1)
		| ((GetEndPointH(0).g & 0xF8) << 4) | ((GetEndPointL(1).g & 0xF8) << 9)
		| ((GetEndPointH(1).g & 0xF8) << 14) | ((GetEndPointL(0).b & 0xF8) << 19)
		| ((GetEndPointH(0).b & 0xF8) << 24);
	Block.z = ((GetEndPointL(1).b & 0xF8) >> 3) | ((GetEndPointH(1).b & 0xF8) << 2)
		| ((GetEndPointL(0).a & 0xF8) << 7) | ((GetEndPointH(0).a & 0xF8) << 12)
		| ((GetEndPointL(1).a & 0xF8) << 17) | ((GetEndPointH(1).a & 0xF8) << 22)
		| ((GetEndPointL(0).r & 0x04) << 28) | ((GetEndPointH(0).r & 0x04) << 29);
	Block.w = ((GetEndPointL(1).r & 0x04) >> 2) | ((GetEndPointH(1).r & 0x04) >> 1)
		| (GetColorIndex(0) << 2);
	
	uint i = 1;
	for (; i <= CandidateFixUpIndex1DOrdered[Partition][0]; i++)
	{
		Block.w |= GetColorIndex(i) << (i * 2 + 1);
	}

	for (; i < 16; i++)
	{
		Block.w |= GetColorIndex(i) << (i * 2);
	}
}

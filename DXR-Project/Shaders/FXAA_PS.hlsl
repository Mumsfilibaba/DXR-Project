#include "PBRCommon.hlsli"

#define FXAA_EDGE_THRESHOLD			(1.0f / 8.0f)
#define FXAA_EDGE_THRESHOLD_MIN		(1.0f / 24.0f)

#define FXAA_SUBPIX_TRIM			(1.0f / 4.0f)
#define FXAA_SUBPIX_CAP				(3.0f / 4.0f)
#define FXAA_SUBPIX_TRIM_SCALE		(1.0f / (1.0f - FXAA_SUBPIX_TRIM))

#define FXAA_SEARCH_THRESHOLD		(1.0f / 4.0f)
#define FXAA_SEARCH_STEPS			32

cbuffer CB0 : register(b0, space0)
{
	float2 TextureSize;
}

Texture2D FinalImage		: register(t0, space0);
SamplerState PointSampler	: register(s0, space0);
SamplerState LinearSampler	: register(s1, space0);

/*
* Helpers
*/
float3 FXAASample(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord)
{
	return Texture.SampleLevel(InSampler, TexCoord, 0).rgb;
}

float3 FXAASampleOffset(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord, int2 Offset)
{
	return Texture.SampleLevel(InSampler, TexCoord, 0, Offset).rgb;
}

float3 FXAASampleGrad(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord, float2 Grad)
{
    return Texture.SampleGrad(InSampler, TexCoord, Grad, Grad).rgb;
}

float FXAALuma(float3 Color)
{
	return sqrt(dot(Color, float3(0.299f, 0.587f, 0.114f)));
}

float3 Lerp(float3 A, float3 B, float AmountOfA)
{
	return (ToFloat3(-AmountOfA) * B) + ((A * ToFloat3(AmountOfA)) + B);
}

/*
* PixelShader
*/
float4 Main(float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	// Correct the texcoords
	TexCoord.y = 1.0f - TexCoord.y;
	
	// Perform edge detection
	const float2 InvTextureSize = float2(1.0f, 1.0f) / TextureSize;
	float3 Middle = FXAASampleOffset(FinalImage, PointSampler, TexCoord, int2(0, 0));
	float3 North	= FXAASampleOffset(FinalImage, PointSampler, TexCoord, int2(0, 1));
	float3 South	= FXAASampleOffset(FinalImage, PointSampler, TexCoord, int2(0, -1));
	float3 West	= FXAASampleOffset(FinalImage, PointSampler, TexCoord, int2(-1, 0));
	float3 East	= FXAASampleOffset(FinalImage, PointSampler, TexCoord, int2(1, 0));
	float LumaM	= FXAALuma(Middle);
	float LumaN	= FXAALuma(North);
	float LumaS	= FXAALuma(South);
	float LumaW	= FXAALuma(West);
	float LumaE	= FXAALuma(East);
	
	float RangeMin = min(LumaM, min(min(LumaN, LumaS), min(LumaW, LumaE)));
	float RangeMax = max(LumaM, max(max(LumaN, LumaS), max(LumaW, LumaE)));
	float Range = RangeMax - RangeMin;
	if (Range < max(FXAA_EDGE_THRESHOLD_MIN, RangeMax * FXAA_EDGE_THRESHOLD))
	{
		return float4(Middle, 1.0f);
	}
	
	float LumaL		= (LumaN + LumaS + LumaW + LumaE) * 0.25f;
	float RangeL	= abs(LumaL - LumaM);
	float BlendL	= max(0.0f, (RangeL / Range) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE;
	BlendL = min(BlendL, FXAA_SUBPIX_CAP);
	
	float3 NorthWest = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(-1, 1));
	float3 SouthWest = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(-1, -1));
	float3 NorthEast = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(1, 1));
	float3 SouthEast = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(1, -1));
	
	float3 ColorL = (Middle + North + South + West + East);
	ColorL += (NorthWest + SouthWest + NorthEast + SouthEast);
	ColorL = ColorL * ToFloat3(1.0f / 9.0f);
	
	float LumaNW = FXAALuma(NorthWest);
	float LumaNE = FXAALuma(NorthEast);
	float LumaSW = FXAALuma(SouthWest);
	float LumaSE = FXAALuma(SouthEast);
	
	float LumaNorthSouth	= LumaN + LumaS;
	float LumaWestEast		= LumaW + LumaE;
	float LumaWestCorners	= LumaSW + LumaNW;
	float LumaSouthCorners	= LumaSW + LumaSE;
	float LumaEastCorners	= LumaSE + LumaNE;
	float LumaNorthCorners	= LumaNW + LumaNE;
	
	float EdgeVert =
		(abs((-2.0f * LumaN) + LumaNorthCorners))		+
		(abs((-2.0f * LumaM) + LumaWestEast) * 2.0f) +
		(abs((-2.0f * LumaS) + LumaSouthCorners));
	float EdgeHorz =
		(abs((-2.0f * LumaW) + LumaWestCorners))		+
		(abs((-2.0f * LumaM) + LumaNorthSouth) * 2.0f)	+
		(abs((-2.0f * LumaE) + LumaEastCorners));
	
	bool IsHorizontal = (EdgeHorz >= EdgeVert);
	if (!IsHorizontal)
	{
		LumaN = LumaW;
	}
	if (!IsHorizontal)
	{
		LumaS = LumaE;
	}
	
	float GradientN = abs(LumaN - LumaM);
	float GradientS = abs(LumaS - LumaM);
	float LumaAvgN = (LumaN + LumaM) * 0.5f;
	float LumaAvgS = (LumaS + LumaM) * 0.5f;
	
	bool PairN = (abs(GradientN) >= abs(GradientS));
	float LocalLumaAvg	= PairN ? LumaAvgN	: LumaAvgS;
	float LocalGradient = PairN ? GradientN	: GradientS;
	LocalGradient = LocalGradient * FXAA_SEARCH_THRESHOLD;

	float StepLength = (!IsHorizontal) ? InvTextureSize.y : -InvTextureSize.x;
	if (PairN)
	{
		StepLength = -StepLength;
	}
	
	float2 CurrentTexCoord = TexCoord;
	if (IsHorizontal)
	{
		CurrentTexCoord.y += StepLength * 0.5f;
	}
	else
	{
		CurrentTexCoord.x += StepLength * 0.5f;
	}
	
	float2 Offset = IsHorizontal ? float2(InvTextureSize.x, 0.0f) : float2(0.0f, InvTextureSize.y);
	bool Done0 = false;
	bool Done1 = false;
	float LumaEnd0;
	float LumaEnd1;
#if 0
	float2 TexCoord0 = CurrentTexCoord - Offset;
	float2 TexCoord1 = CurrentTexCoord + Offset;
#else
    float2 TexCoord0 = CurrentTexCoord - Offset * float2(2.5f, 2.5f);
    float2 TexCoord1 = CurrentTexCoord + Offset * float2(2.5f, 2.5f);
    Offset *= float2(4.0f, 4.0f);
#endif
    for (int i = 0; i < FXAA_SEARCH_STEPS; i++)
	{
#if 0
		if (!Done0)
		{
			LumaEnd0 = FXAALuma(FXAASample(FinalImage, LinearSampler, TexCoord0).rgb);
		}
		if(!Done1)
		{
			LumaEnd1 = FXAALuma(FXAASample(FinalImage, LinearSampler, TexCoord1).rgb);
		}
#else
        if (!Done0)
        {
            LumaEnd0 = FXAALuma(FXAASampleGrad(FinalImage, LinearSampler, TexCoord0, Offset).rgb);
        }
        if (!Done1)
        {
            LumaEnd1 = FXAALuma(FXAASampleGrad(FinalImage, LinearSampler, TexCoord1, Offset).rgb);
        }
#endif
		
		Done0 = (abs(LumaEnd0 - LocalLumaAvg) >= LocalGradient);
		Done1 = (abs(LumaEnd1 - LocalLumaAvg) >= LocalGradient);
		if (Done0 && Done1)
		{
			break;
		}
		
		if (!Done0)
		{
            TexCoord0 -= Offset;
        }
		if (!Done1)
		{
            TexCoord1 += Offset;
        }
	}
	
	float Distance0 = IsHorizontal ? (TexCoord.x - TexCoord0.x) : (TexCoord.y - TexCoord0.y);
	float Distance1 = IsHorizontal ? (TexCoord1.x - TexCoord.x) : (TexCoord1.y - TexCoord.y);
	bool Direction0	= Distance0 < Distance1;
	LumaEnd0 = Direction0 ? LumaEnd0 : LumaEnd0;
	
	if (((LumaM - LumaN) < 0.0f) == ((LumaEnd0 - LumaN) < 0.0f))
	{
		StepLength = 0.0f;
	}
	
	float SpanLength = (Distance0 + Distance1);
	Distance0 = Direction0 ? Distance0 : Distance1;
	
	float SubPixelOffset = (0.5f + (Distance0 * (-1.0f / SpanLength))) * StepLength;
	float2 FinalTexCoord = TexCoord + float2(IsHorizontal ? 0.0f : SubPixelOffset, IsHorizontal ? SubPixelOffset : 0.0f);
	float3 ColorF = FXAASample(FinalImage, LinearSampler, FinalTexCoord).rgb;
    float3 FinalColor = Lerp(ColorL, ColorF, BlendL);
    return float4(FinalColor, 1.0f);
}
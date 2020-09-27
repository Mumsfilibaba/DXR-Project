#include "PBRCommon.hlsli"

#define FXAA_EDGE_THRESHOLD			(1.0f / 8.0f)
#define FXAA_EDGE_THRESHOLD_MIN		(1.0f / 24.0f)

#define FXAA_SUBPIX_TRIM			(1.0f / 4.0f)
#define FXAA_SUBPIX_CAP				(3.0f / 4.0f)
#define FXAA_SUBPIX_TRIM_SCALE		(1.0f / (1.0f - FXAA_SUBPIX_TRIM))

#define FXAA_SEARCH_THRESHOLD		(1.0f / 4.0f)
#define FXAA_SEARCH_STEPS			24

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
float4 FXAASample(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord)
{
	return Texture.SampleLevel(InSampler, TexCoord, 0);
}

float4 FXAASampleOffset(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord, int2 Offset)
{
	return Texture.SampleLevel(InSampler, TexCoord, 0, Offset);
}

float4 FXAASampleGrad(in Texture2D Texture, in SamplerState InSampler, float2 TexCoord, float2 Grad)
{
	return Texture.SampleGrad(InSampler, TexCoord, Grad, Grad);
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
	float4 Middle = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(0, 0));
	float4 North = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(0, 1));
	float4 South = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(0, -1));
	float4 West = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(-1, 0));
	float4 East = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(1, 0));
	float LumaM = Middle.a;
	float LumaN = North.a;
	float LumaS = South.a;
	float LumaW = West.a;
	float LumaE = East.a;
	
	float RangeMin = min(LumaM, min(min(LumaN, LumaS), min(LumaW, LumaE)));
	float RangeMax = max(LumaM, max(max(LumaN, LumaS), max(LumaW, LumaE)));
	float Range = RangeMax - RangeMin;
	if (Range < max(FXAA_EDGE_THRESHOLD_MIN, RangeMax * FXAA_EDGE_THRESHOLD))
	{
		return float4(Middle.rgb, 1.0f);
	}
	
	float LumaL		= (LumaN + LumaS + LumaW + LumaE) * 0.25f;
	float RangeL	= abs(LumaL - LumaM);
	float BlendL	= max(0.0f, (RangeL / Range) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE;
	BlendL = min(BlendL, FXAA_SUBPIX_CAP);
	
	float4 NorthWest = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(-1, 1));
	float4 SouthWest = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(-1, -1));
	float4 NorthEast = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(1, 1));
	float4 SouthEast = FXAASampleOffset(FinalImage, LinearSampler, TexCoord, int2(1, -1));
	
	float3 ColorL = (Middle + North + South + West + East);
	ColorL += (NorthWest + SouthWest + NorthEast + SouthEast);
	ColorL = ColorL * ToFloat3(1.0f / 9.0f);
	
	float LumaNW = NorthWest.a;
	float LumaNE = NorthEast.a;
	float LumaSW = SouthWest.a;
	float LumaSE = SouthEast.a;
	
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
	
	int Steps = 0;
	for (; Steps < FXAA_SEARCH_STEPS; Steps++)
	{
#if 0
		if (!Done0)
		{
			float4 Sample = FXAASample(FinalImage, LinearSampler, TexCoord0);
			LumaEnd0 = Sample.a;
		}
		if(!Done1)
		{
			float4 Sample = FXAASample(FinalImage, LinearSampler, TexCoord1);
			LumaEnd1 = Sample.a;
		}
#else
		if (!Done0)
		{
			float4 Sample = FXAASampleGrad(FinalImage, LinearSampler, TexCoord0, Offset);
			LumaEnd0 = Sample.a;
		}
		if (!Done1)
		{
			float4 Sample = FXAASampleGrad(FinalImage, LinearSampler, TexCoord1, Offset);
			LumaEnd1 = Sample.a;
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
	
	float SubPixelOffset = (0.5f + (Distance0 * (1.0f / SpanLength))) * StepLength;
	float2 FinalTexCoord = TexCoord + float2(IsHorizontal ? 0.0f : SubPixelOffset, IsHorizontal ? SubPixelOffset : 0.0f);
	
	float3 ColorF = FXAASample(FinalImage, LinearSampler, FinalTexCoord).rgb;
	float3 FinalColor = Lerp(ColorL, ColorF, BlendL);
	return float4(FinalColor, 1.0f);
}